# Manual-test driver library for warehouse-mfc.
# Dot-source this in each step:  . "C:\...\tests\manual\uia.ps1"
# Uses UI Automation (System.Windows.Automation) + Win32 mouse injection.

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Native {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int w, int ht, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern short VkKeyScan(char ch);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    public struct RECT { public int Left, Top, Right, Bottom; }
    public const uint MOUSEEVENTF_LEFTDOWN = 0x0002, MOUSEEVENTF_LEFTUP = 0x0004;
    public const uint MOUSEEVENTF_RIGHTDOWN = 0x0008, MOUSEEVENTF_RIGHTUP = 0x0010;
}
"@

[void][Native]::SetProcessDPIAware()  # one physical-pixel space for rect/mouse/screenshot/UIA

$global:AppRoot = "C:\Users\synap\Claude\warehouse-mfc"
$global:ShotDir = "$AppRoot\tests\manual\shots"

function Get-AppHwnd {
    (Get-Process app -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1).MainWindowHandle
}

function Pin-App {
    # The app restores a ~1102px height on activation (off-screen). Activate first, then force
    # the on-screen size LAST with NOACTIVATE, retrying until it sticks.
    param([int]$X=0,[int]$Y=0,[int]$W=1280,[int]$H=1015)
    $h = Get-AppHwnd
    if (-not $h) { return }
    [void][Native]::BringWindowToTop($h)
    [void][Native]::SetForegroundWindow($h)
    Start-Sleep -Milliseconds 250
    for ($i=0; $i -lt 6; $i++) {
        [void][Native]::SetWindowPos($h, [IntPtr]::Zero, $X, $Y, $W, $H, 0x0050)  # SHOWWINDOW|NOACTIVATE
        Start-Sleep -Milliseconds 150
        $r = New-Object Native+RECT; [void][Native]::GetWindowRect($h,[ref]$r)
        if (($r.Bottom-$r.Top) -eq $H -and ($r.Right-$r.Left) -eq $W) { break }
    }
}

# Re-assert size right before a shot (call after any click that may have re-activated).
function Fix-Size {
    $h = Get-AppHwnd
    for ($i=0; $i -lt 6; $i++) {
        $r = New-Object Native+RECT; [void][Native]::GetWindowRect($h,[ref]$r)
        if (($r.Bottom-$r.Top) -eq 1015 -and $r.Top -eq 0) { return }
        [void][Native]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 1280, 1015, 0x0050)
        Start-Sleep -Milliseconds 150
    }
}

function Get-AppElement {
    $h = Get-AppHwnd
    [System.Windows.Automation.AutomationElement]::FromHandle($h)
}

# Dump the UIA subtree (name/type/bounds) for discovery.
function Dump-Tree {
    param($El, [int]$Depth=0, [int]$Max=6, [switch]$Raw)
    if ($Depth -gt $Max -or $null -eq $El) { return }
    try {
        $name = $El.Current.Name
        $ct = $El.Current.ControlType.ProgrammaticName -replace 'ControlType\.',''
        $r = $El.Current.BoundingRectangle
        $auto = $El.Current.AutomationId
        $pad = ' ' * ($Depth*2)
        Write-Output ("{0}[{1}] '{2}' {3} ({4},{5} {6}x{7})" -f $pad, $ct, $name, $auto, [int]$r.X, [int]$r.Y, [int]$r.Width, [int]$r.Height)
    } catch {}
    $walker = if ($Raw) { [System.Windows.Automation.TreeWalker]::RawViewWalker } else { [System.Windows.Automation.TreeWalker]::ControlViewWalker }
    $child = $walker.GetFirstChild($El)
    while ($null -ne $child) {
        Dump-Tree $child ($Depth+1) $Max -Raw:$Raw
        $child = $walker.GetNextSibling($child)
    }
}

# Find first element by name (substring) and optional control type.
function Find-El {
    param([string]$Name, [string]$Type)
    $root = Get-AppElement
    $walker = [System.Windows.Automation.TreeWalker]::ControlViewWalker
    $stack = New-Object System.Collections.Stack
    $stack.Push($root)
    while ($stack.Count -gt 0) {
        $el = $stack.Pop()
        try {
            $n = $el.Current.Name
            $t = $el.Current.ControlType.ProgrammaticName
            if ($n -like "*$Name*" -and (-not $Type -or $t -like "*$Type*")) { return $el }
        } catch {}
        $c = $walker.GetFirstChild($el)
        while ($null -ne $c) { $stack.Push($c); $c = $walker.GetNextSibling($c) }
    }
    return $null
}

function Center-Of { param($El) $r=$El.Current.BoundingRectangle; return @([int]($r.X+$r.Width/2),[int]($r.Y+$r.Height/2)) }

function Click-Point { param([int]$X,[int]$Y,[switch]$Right)
    [void][Native]::SetCursorPos($X,$Y); Start-Sleep -Milliseconds 120
    if ($Right) { [Native]::mouse_event([Native]::MOUSEEVENTF_RIGHTDOWN,0,0,0,[UIntPtr]::Zero); Start-Sleep -Milliseconds 60; [Native]::mouse_event([Native]::MOUSEEVENTF_RIGHTUP,0,0,0,[UIntPtr]::Zero) }
    else { [Native]::mouse_event([Native]::MOUSEEVENTF_LEFTDOWN,0,0,0,[UIntPtr]::Zero); Start-Sleep -Milliseconds 60; [Native]::mouse_event([Native]::MOUSEEVENTF_LEFTUP,0,0,0,[UIntPtr]::Zero) }
    Start-Sleep -Milliseconds 350
}

function Click-El { param($El,[switch]$Right) $p = Center-Of $El; Click-Point $p[0] $p[1] -Right:$Right }

function Invoke-El { param($El)
    try { $p=$El.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern); $p.Invoke(); Start-Sleep -Milliseconds 350; return $true } catch { return $false }
}

# Full-screen shot (captures popups/menus that live outside the app window).
function Shot-Screen { param([string]$Name)
    $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($b.X,$b.Y,0,0,$bmp.Size)
    $path = "$ShotDir\$Name.png"; $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
    Write-Output "shot $path"
}

# Window-only shot.
function Shot-App { param([string]$Name)
    $h = Get-AppHwnd; $r = New-Object Native+RECT; [void][Native]::GetWindowRect($h,[ref]$r)
    $w=$r.Right-$r.Left; $ht=$r.Bottom-$r.Top
    $bmp = New-Object System.Drawing.Bitmap $w,$ht; $g=[System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.Left,$r.Top,0,0,(New-Object System.Drawing.Size $w,$ht))
    $path="$ShotDir\$Name.png"; $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
    Write-Output "shot $path ($w x $ht)"
}

function Send-Key { param([string]$Keys) [System.Windows.Forms.SendKeys]::SendWait($Keys); Start-Sleep -Milliseconds 250 }
