# Codified exploratory capture sweep for warehouse-mfc.
# One command walks a comprehensive scenario and screenshots each step into shots/ for a
# by-eye / AI review. This makes the *capture* repeatable and deterministic; the *assessment*
# is still visual (themes, owner-draw, layout — the cases UIA can't assert; see tests/ui for
# the parts that are machine-asserted).
#
#   powershell -File tests\manual\sweep.ps1
#
# The undo/redo cycle records a movement and immediately undoes it (net-zero), so the demo
# DB is left unchanged. Everything else cancels out.
#
# -Lang pl|en forces the UI language for this run (registry, applied on the launch below);
# -Prefix names the shots (e.g. sweep-en-01-main). NOTE: ribbon hit-points are tuned for the
# Polish layout; English labels are shorter so some interaction clicks may land slightly off —
# the captured screenshots still show the English layout for a by-eye review, which is the point.

param([ValidateSet('', 'pl', 'en')] [string]$Lang = '', [string]$Prefix = 'sweep')

. "$PSScriptRoot\uia.ps1"

if ($Lang) {
    $code = if ($Lang -eq 'en') { 1 } else { 0 }
    reg add "HKCU\Software\warehouse-mfc\app\Settings" /v Language /t REG_DWORD /d $code /f | Out-Null
    Stop-Process -Name app -Force -ErrorAction SilentlyContinue   # force relaunch in the chosen language
    Start-Sleep -Milliseconds 500
}

# Ribbon/grid hit points in pinned-window (1280-wide) pixel coordinates. UIA can't see the
# MFC ribbon or grid rows, so these are read from screenshots (re-probe if the layout changes).
$TabMagazyn = @(104, 50);  $TabWidok = @(178, 50)   # shifted right by the ribbon application button
$Odswiez = @(38, 100); $TylkoNiskie = @(102, 100); $Przyjmij = @(158, 108); $Wydaj = @(207, 100)
$Cofnij = @(270, 100); $Ponow = @(320, 100)
$Motyw = @(40, 105); $TogglePulpit = @(110, 110); $ToggleDziennik = @(155, 110); $ToggleSzczegoly = @(210, 110)
$HdrMagazyn = @(340, 192); $HdrSymbol = @(420, 192); $HdrProdukt = @(565, 192); $HdrStan = @(710, 192)
$Row1 = @(450, 212); $Row4 = @(450, 272)

function Tap($p) { Click-Point $p[0] $p[1] }
$script:step = 0
function Capture([string]$name, [switch]$Screen) {
    $script:step++
    $tag = "{0}-{1:00}-{2}" -f $Prefix, $script:step, $name
    if ($Screen) { Shot-Screen $tag } else { Shot-App $tag }
}

Ensure-App
Send-Key "{ESC}"          # clear any leftover modal
Tap $TabMagazyn; Pin-App
Capture "main"

# --- Sorting: every column, both directions (header clicks) ---
Tap $HdrStan;    Capture "sort-stan-asc"
Tap $HdrStan;    Capture "sort-stan-desc"
Tap $HdrProdukt; Capture "sort-produkt"
Tap $HdrMagazyn; Capture "sort-magazyn"

# --- Selection: a data row fills the Details pane ---
Tap $Row4; Capture "selection-details"

# --- Filter: low-stock only, then off ---
Tap $TylkoNiskie; Capture "filter-low-on"
Tap $TylkoNiskie; Capture "filter-low-off"

# --- Record dialogs (cancelled): pre-select follows the selected row ---
Tap $Row1
Tap $Przyjmij; Capture "record-in-preselect"; Close-Dialog
Tap $Wydaj;    Capture "record-out";          Close-Dialog

# --- Validation: zero quantity is rejected, modal stays up ---
Tap $Przyjmij
Send-Key "0"; Send-Key "{ENTER}"
Capture "validation-msgbox" -Screen          # full-screen: the modal lives outside the window
Send-Key "{ENTER}"; Close-Dialog

# --- Undo / redo cycle (records +1, then restores net-zero) ---
# Undo/redo go through the Ctrl+Z / Ctrl+Y accelerators, not the ribbon Cofnij/Ponów buttons:
# the buttons enable a frame late, so a pixel click can miss; the accelerator always routes.
Tap $Row1
Tap $Przyjmij; Send-Key "{ENTER}"            # accept default qty (1) -> records IN
Capture "after-record"
Send-Key "^z"; Capture "after-undo"
Send-Key "^y"; Capture "after-redo"
Send-Key "^z"                                # final undo -> back to the seeded state

# --- Themes: open the Motyw menu, switch to dark, restore light ---
Tap $TabWidok
Tap $Motyw; Capture "theme-menu" -Screen
Send-Key "{END}"; Send-Key "{ENTER}"; Capture "theme-dark"
Tap $Motyw; Send-Key "{HOME}"; Send-Key "{ENTER}"; Capture "theme-light-restored"

# --- Panes: hide/show a dockable pane; the centre grid reflows ---
Tap $ToggleSzczegoly; Capture "pane-hidden"
Tap $ToggleSzczegoly; Capture "pane-restored"
Tap $TabMagazyn

# --- Robustness: extreme shrink, then restore ---
$h = Get-AppHwnd
[void][Native]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 720, 480, 0x0050)
Start-Sleep -Milliseconds 300
Capture "resize-tiny"
Pin-App
Capture "resize-restored"

Write-Output ("`nSweep complete: {0} screenshots in {1}" -f $script:step, $ShotDir)

# Always close the app when the sweep finishes (no leftover window/modal to interfere
# with the next run or the foreground).
Send-Key "{ESC}"                                  # dismiss any stray modal first
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
Write-Output "App closed."
