<#
  Local "CI" runner - one command for every test layer of warehouse-mfc.

    core_tests   unit tests (Catch2)            -> exit-code gate
    data_smoke   data/ smoke vs LocalDB         -> exit-code gate
    tests/ui     Pester + UIA assertions        -> exit-code gate
    sweep pl/en  visual exploratory capture     -> informational (review shots/ by eye/AI)

  The app must be built (or pass -Build). The DB is reset to a clean baseline before and after,
  and the saved language is cleared, so the run is deterministic and self-contained.

  Usage:
    powershell -File run-tests.ps1                 # run everything (assumes already built)
    powershell -File run-tests.ps1 -Build          # build Debug + Release first
    powershell -File run-tests.ps1 -NoSweep         # gates only (skip the visual sweeps)
    powershell -File run-tests.ps1 -Config Debug    # use the Debug binaries

  Exit code = number of failed gating layers (0 = all green).
#>
param(
    [switch]$Build,
    [ValidateSet('Release', 'Debug')] [string]$Config = 'Release',
    [switch]$NoSweep,
    [switch]$AiReview   # after the sweep, ask the `claude` CLI to eyeball the shots
)

$root = $PSScriptRoot
$gates = [ordered]@{}   # name -> bool (pass), counted in the exit code
$sweeps = [ordered]@{}  # name -> bool (ran)   informational

function Get-LocalDbPipe {
    (sqllocaldb info MSSQLLocalDB | Select-String 'Instance pipe name').ToString().Split(':', 2)[1].Trim()
}
function Reset-Db {
    sqlcmd -S (Get-LocalDbPipe) -d Warehouse -i "$root\db\02_seed.sql" | Out-Null
}
function Reset-Lang {
    reg delete "HKCU\Software\warehouse-mfc" /f 2>$null | Out-Null   # back to auto-detect
}
function Section($t) { Write-Host "`n== $t ==" -ForegroundColor Cyan }

if ($Build) {
    $msbuild = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path $msbuild)) { $msbuild = (Get-Command msbuild -ErrorAction SilentlyContinue).Source }
    if (-not $msbuild) { throw "msbuild not found - open a Developer prompt or pass the VS path." }
    foreach ($cfg in 'Debug', 'Release') {
        Section "build $cfg"
        & $msbuild "$root\warehouse-mfc.sln" /p:Configuration=$cfg /p:Platform=x64 /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw "build $cfg failed" }
    }
}

Stop-Process -Name app -Force -ErrorAction SilentlyContinue
Reset-Db          # deterministic baseline (also resets IDENTITY -> ids 1..N)
Reset-Lang

# --- gating layers ---
Section 'core_tests'
& "$root\x64\$Config\core_tests.exe"
$gates['core_tests'] = ($LASTEXITCODE -eq 0)

Section 'data_smoke'
& "$root\x64\$Config\data_smoke.exe"
$gates['data_smoke'] = ($LASTEXITCODE -eq 0)

Section 'tests/ui'
Stop-Process -Name app -Force -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 500
& powershell -NoProfile -ExecutionPolicy Bypass -File "$root\tests\ui\Run.ps1"
$gates['tests/ui'] = ($LASTEXITCODE -eq 0)

# --- visual sweeps (informational) ---
if (-not $NoSweep) {
    foreach ($lang in 'pl', 'en') {
        Section "sweep ($lang)"
        Stop-Process -Name app -Force -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 500
        & powershell -NoProfile -ExecutionPolicy Bypass -File "$root\tests\manual\sweep.ps1" -Lang $lang -Prefix "sweep-$lang" |
            Select-Object -Last 1
        $sweeps["sweep-$lang"] = (Test-Path "$root\tests\manual\shots\sweep-$lang-01-main.png")
    }
}

# --- AI review of the sweep shots (headless: the `claude` CLI reads the PNGs) ---
if ($AiReview -and -not $NoSweep) {
    Section 'AI review (sweep shots)'
    if (-not (Get-Command claude -ErrorAction SilentlyContinue)) {
        Write-Host "  'claude' CLI not on PATH - skipping (or review tests/manual/shots by eye)." -ForegroundColor Yellow
    } else {
        $prompt = @"
You are reviewing screenshots of a warehouse-stock MFC desktop app, captured by an automated
sweep in two languages. Read EVERY file matching tests/manual/shots/sweep-pl-*.png (Polish UI)
and tests/manual/shots/sweep-en-*.png (English UI). Each filename ends in NN-name; the NN-name
identifies the captured state. Both runs share the same NN-name set, so judge each shot against
its expectation below, and additionally check that the EN run is fully translated.

Ground rules (avoid false positives):
- Product and warehouse NAMES stay Polish in BOTH runs - they are database data, not UI. Do NOT
  report them as untranslated. Only app chrome (ribbon, panels, columns, dialogs, status bar,
  messages) must switch language between the pl and en runs.
- Em dashes / accented Polish letters rendering correctly is expected; only flag truly garbled
  (mojibake) glyphs.

Per-shot expectations (apply to the matching NN-name in BOTH the pl and en runs):
- 01-main: ribbon + grid + 3 docked panes render; columns not clipped; app-icon orb top-left;
  status bar shows an item count.
- 02-sort-stan-asc: grid sorted by the on-hand column ascending, NUMERICALLY (e.g. 9 before 10).
- 03-sort-stan-desc: same column sorted descending, NUMERICALLY (not lexicographically).
- 04-sort-produkt: rows sorted alphabetically by product name.
- 05-sort-magazyn: rows sorted alphabetically by warehouse name.
- 06-selection-details: a grid row is selected and the Details pane shows that row's fields
  (on-hand value, low-stock yes/no).
- 07-filter-low-on: grid narrowed to low-stock rows only; status shows 'N of total' (PL 'N z 10',
  EN 'N of 10'), with N < total.
- 08-filter-low-off: full row set restored; status back to the full count.
- 09-record-in-preselect: a modal receipt dialog (PL 'Przyjecie (IN)', EN 'Receipt (IN)'); its
  Product and Warehouse combos are PRE-FILLED from the selected row (NOT empty); labels and OK/
  Cancel buttons localised; dialog matches the light theme.
- 10-record-out: modal issue dialog (PL 'Wydanie (OUT)', EN 'Issue (OUT)'); otherwise as above.
- 11-validation-msgbox: a message box rejects quantity 0 (PL 'od 1 do', EN 'between 1 and'); the
  record dialog is STILL OPEN behind the message box (the modal was not dismissed).
- 12-context-menu: a right-click popup over a grid row lists row actions (Receive/Issue/Refresh/
  Details, localised).
- 13-about: the About dialog (PL 'O programie', EN 'About') with description, 'Version 1.0', and a
  shortcuts line.
- 14-after-record: same state as 01 but the previously selected row's on-hand is +1, a new entry
  appears in the movement journal pane, AND the Details pane's on-hand value matches the grid's new
  value for that row (the Details pane must not show a stale pre-record number).
- 15-after-undo: on-hand back to the 01-main value (undo worked).
- 16-after-redo: on-hand +1 again (redo worked).
- 17-view-tab: the View ribbon tab showing Restore-layout / Language / Help buttons, each with an
  icon; orb still present.
- 18-theme-menu: the Theme dropdown is open listing several themes including a 'Dark (full)' entry.
- 19-theme-dark: the ENTIRE app is dark - ribbon, grid, panes AND the window title bar (dark DWM
  caption). Flag any light patch as a defect.
- 20-theme-light-restored: fully back to the light theme, no dark remnants.
- 21-pane-hidden: the Details pane is hidden and the centre grid has reflowed wider to fill the gap.
- 22-pane-restored: the pane is back and the grid reflowed to its original width.
- 23-resize-tiny: window shrunk to ~720x480; no crash; layout scales without catastrophic overlap.
- 24-resize-restored: clean return to the full 1280-wide layout.

Cross-cutting on every shot: no clipped/overlapping text, no truncated headers, no missing icons,
no mojibake, and (en run only) no Polish UI chrome leaking through.

Report format: first line exactly 'VERDICT: CLEAN' if every shot meets its expectation, otherwise
'VERDICT: ISSUES' followed by a bullet list of '<shot-name>: <what is wrong vs expected>'. Only
list real deviations from the expectations above.
"@
        '' | claude -p $prompt   # empty stdin so the CLI doesn't wait for piped input
    }
}

# --- cleanup ---
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
Reset-Db
Reset-Lang

# --- summary ---
Write-Host "`n================ SUMMARY ================" -ForegroundColor Cyan
$failed = 0
foreach ($k in $gates.Keys) {
    $ok = $gates[$k]; if (-not $ok) { $failed++ }
    Write-Host ("  {0,-12} {1}" -f $k, $(if ($ok) { 'PASS' } else { 'FAIL' })) -ForegroundColor $(if ($ok) { 'Green' } else { 'Red' })
}
foreach ($k in $sweeps.Keys) {
    Write-Host ("  {0,-12} {1}" -f $k, $(if ($sweeps[$k]) { 'captured -> tests/manual/shots (review by eye)' } else { 'DID NOT RUN' })) -ForegroundColor Yellow
}
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ("$($gates.Count - $failed)/$($gates.Count) gating layers passed.") -ForegroundColor $(if ($failed) { 'Red' } else { 'Green' })
exit $failed
