# Manual-test driver library for warehouse-mfc.
# Dot-source this in each step:  . "C:\...\tests\manual\uia.ps1"
# Uses UI Automation (System.Windows.Automation) + Win32 mouse injection.

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

if (-not ([System.Management.Automation.PSTypeName]'Native').Type) {
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
}

[void][Native]::SetProcessDPIAware()  # one physical-pixel space for rect/mouse/screenshot/UIA

if (-not ([System.Management.Automation.PSTypeName]'PopupMenu').Type) {
Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public class PopupMenu {  // detect a tracking context menu (a visible #32768 window)
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr l);
    [DllImport("user32.dll")] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    delegate bool EnumProc(IntPtr h, IntPtr l);
    static int _count; static EnumProc _cb = Cb;
    static bool Cb(IntPtr h, IntPtr l) {
        if (IsWindowVisible(h)) { var c = new StringBuilder(64); GetClassName(h, c, 64); if (c.ToString() == "#32768") _count++; }
        return true;
    }
    public static int Count() { _count = 0; EnumWindows(_cb, IntPtr.Zero); return _count; }
}
"@
}

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

# Fast double-click (two clicks inside the system double-click time) at a screen point.
function DblClick-Point { param([int]$X,[int]$Y)
    [void][Native]::SetCursorPos($X,$Y); Start-Sleep -Milliseconds 90
    [Native]::mouse_event([Native]::MOUSEEVENTF_LEFTDOWN,0,0,0,[UIntPtr]::Zero); [Native]::mouse_event([Native]::MOUSEEVENTF_LEFTUP,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 70
    [Native]::mouse_event([Native]::MOUSEEVENTF_LEFTDOWN,0,0,0,[UIntPtr]::Zero); [Native]::mouse_event([Native]::MOUSEEVENTF_LEFTUP,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 350
}

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

# --- Assertion helpers (used by the Pester specs under tests/ui) -------------
# These read control *state* via UIA (not pixels), so the checks are machine-verifiable.

# Dialog-control automation ids (mirror app/resource.h IDC_PRODUCT/IDC_WAREHOUSE/IDC_QTY).
$global:IdcProduct   = "1001"
$global:IdcWarehouse = "1002"
$global:IdcQty       = "1003"
# Ribbon "Przyjmij" button, in pinned-window pixel coordinates.
$global:RecordInPoint = @(158, 110)

# Launch the Release app if it isn't running, then pin it to a known size/position.
function Ensure-App {
    if (-not (Get-Process app -ErrorAction SilentlyContinue)) {
        Start-Process "$AppRoot\app\x64\Release\app.exe"
        Start-Sleep -Seconds 4
    }
    Pin-App
}

# The modal record-movement dialog, or $null. Matches either language's title
# (PL "Przyjęcie (IN)" / "Wydanie (OUT)", EN "Receipt (IN)" / "Issue (OUT)").
function Find-RecordDialog {
    foreach ($title in @("Przyj", "Wydanie", "Receipt", "Issue")) {
        $d = Find-El $title "Window"
        if ($d) { return $d }
    }
    return $null
}

function Test-RecordDialogOpen { return ($null -ne (Find-RecordDialog)) }

# True while a context (popup) menu is on screen.
function Test-ContextMenuOpen { return ([PopupMenu]::Count() -gt 0) }

# Read a dialog control's text (a combo's selected item, or the quantity) by automation id.
function Read-DlgField {
    param([string]$AutomationId)
    $d = Find-RecordDialog
    if (-not $d) { return $null }
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::AutomationIdProperty, $AutomationId)
    $el = $d.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
    if ($el) { return $el.Current.Name } else { return $null }
}

# Poll until a condition holds (or time out) — replaces fixed sleeps so assertions don't race
# the UI thread. Returns $true if the condition became true, $false on timeout.
function Wait-Until {
    param([scriptblock]$Condition, [int]$TimeoutMs = 4000, [int]$PollMs = 150)
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)
    do {
        if (& $Condition) { return $true }
        Start-Sleep -Milliseconds $PollMs
    } while ((Get-Date) -lt $deadline)
    return $false
}

function Open-RecordIn { Click-Point $RecordInPoint[0] $RecordInPoint[1]; Start-Sleep -Milliseconds 500 }
function Close-Dialog  { Send-Key "{ESC}"; Start-Sleep -Milliseconds 250 }
function Select-GridRow { param([int]$Y = 212) Click-Point 450 $Y }  # click a data row in the main grid
