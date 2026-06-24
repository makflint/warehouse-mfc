# Captures the README demo flow as an ordered frame sequence for an animated GIF.
# Window-only shots at one fixed size, so every frame has identical dimensions (a hard
# requirement for the GIF). The flow is a clean loop: it starts on the Magazyn tab (light,
# seeded DB) and returns there, recording a movement and undoing it net-zero so the demo
# DB is left untouched.
#
#   powershell -File tests\manual\demo_gif.ps1            # PL (default)
#   powershell -File tests\manual\demo_gif.ps1 -Lang en
#
# Frames land in tests\manual\gifframes\f01..fNN.png; assemble with ffmpeg (see README/TODO).

param([ValidateSet('pl','en')] [string]$Lang = 'pl')

. "$PSScriptRoot\uia.ps1"

$FrameDir = "$AppRoot\tests\manual\gifframes"
if (Test-Path $FrameDir) { Remove-Item "$FrameDir\*.png" -Force -ErrorAction SilentlyContinue }
else { New-Item -ItemType Directory -Path $FrameDir | Out-Null }

# Force the UI language for this run (registry, applied on the launch below).
$code = if ($Lang -eq 'en') { 1 } else { 0 }
reg add "HKCU\Software\warehouse-mfc\app\Settings" /v Language /t REG_DWORD /d $code /f | Out-Null
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

# Ribbon/grid hit points in pinned-window (1280-wide) pixel coordinates (from sweep.ps1).
$TabMagazyn = @(104, 50);  $TabWidok = @(178, 50)
$Przyjmij   = @(158, 108)
$Motyw      = @(40, 105)
$Row1       = @(450, 212)

function Tap($p) { Click-Point $p[0] $p[1] }

$script:n = 0
function Frame([string]$label) {
    $script:n++
    Fix-Size                                   # keep every frame the same size
    $tag = "f{0:00}-{1}" -f $script:n, $label
    $h = Get-AppHwnd; $r = New-Object Native+RECT; [void][Native]::GetWindowRect($h,[ref]$r)
    $w = $r.Right-$r.Left; $ht = $r.Bottom-$r.Top
    $bmp = New-Object System.Drawing.Bitmap $w,$ht; $g=[System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.Left,$r.Top,0,0,(New-Object System.Drawing.Size $w,$ht))
    $bmp.Save("$FrameDir\$tag.png",[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
    Write-Output "frame $tag ($w x $ht)"
}

Ensure-App
Send-Key "{ESC}"
Tap $TabMagazyn; Pin-App
Frame "main"

# Select a row -> the Szczegoly (Details) pane fills.
Tap $Row1; Frame "select"

# Record a movement (Przyjmij / IN), pre-selected to the row, accept the default qty.
Tap $Przyjmij;        Frame "dialog"
Send-Key "{ENTER}";   Frame "recorded"      # grid on-hand bumps, dashboard bar grows

# Undo / redo the movement through the core CommandStack (Ctrl+Z / Ctrl+Y).
Send-Key "^z";        Frame "undo"
Send-Key "^y";        Frame "redo"
Send-Key "^z"                                # final undo -> back to seeded state (no capture)

# Theme switch: open the Motyw menu, pick the last item (dark), then restore the first (light).
Tap $TabWidok;        Frame "viewtab"
Tap $Motyw; Send-Key "{END}";  Send-Key "{ENTER}"; Frame "dark"
Tap $Motyw; Send-Key "{HOME}"; Send-Key "{ENTER}"; Frame "light"

# Back to the start state (Magazyn, light, clean) so the GIF loops seamlessly.
Tap $TabMagazyn;      Frame "loop"

Write-Output ("`nCaptured {0} frames in {1}" -f $script:n, $FrameDir)
Send-Key "{ESC}"
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
Write-Output "App closed."
