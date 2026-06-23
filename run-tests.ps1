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
    [switch]$NoSweep
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
