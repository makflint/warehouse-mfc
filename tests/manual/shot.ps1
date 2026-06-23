# Manual-test screenshot helper.
# Usage: powershell -File shot.ps1 -Name app -Out shots\01.png
param(
    [string]$Name = 'app',
    [string]$Out  = 'shot.png'
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool f);
    [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
    public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$proc = Get-Process -Name $Name -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $proc) { Write-Error "No window for process '$Name'"; exit 1 }
$h = $proc.MainWindowHandle

# Force foreground so MFC dock panes / ribbon actually paint.
$fg = [Win]::GetForegroundWindow()
$tCur = [Win]::GetCurrentThreadId()
$tFg  = [Win]::GetWindowThreadProcessId($fg, [ref]([uint32]0))
[void][Win]::AttachThreadInput($tCur, $tFg, $true)
[void][Win]::ShowWindow($h, 9)   # SW_RESTORE
[void][Win]::SetForegroundWindow($h)
[void][Win]::AttachThreadInput($tCur, $tFg, $false)
Start-Sleep -Milliseconds 700

$r = New-Object Win+RECT
[void][Win]::GetWindowRect($h, [ref]$r)
$w = $r.Right - $r.Left
$ht = $r.Bottom - $r.Top
if ($w -le 0 -or $ht -le 0) { Write-Error "Bad window rect"; exit 1 }

$bmp = New-Object System.Drawing.Bitmap $w, $ht
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size $w, $ht))
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "saved $Out ($w x $ht)"
