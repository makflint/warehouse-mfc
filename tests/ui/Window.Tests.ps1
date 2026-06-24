# Assertion tests for window-level behaviour: the OUT dialog (sweep step 10), dockable-pane
# hide/show with grid reflow (18-19), and extreme-resize robustness (20-21). These read state
# via UIA + Win32 window queries — no screenshots.

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. "$here\..\manual\uia.ps1"
. "$here\gridreader.ps1"

Describe "Window and panes" {

    Ensure-App; Send-Key "{ESC}"; Click-Point 104 50; Pin-App   # Magazyn tab
    $g = Get-MainGrid

    It "opens the OUT dialog titled Wydanie" {
        Click-Point 450 212
        Click-Point 207 100                       # Wydaj
        [void](Wait-Until { $null -ne (Find-El "Wydanie" "Window") })
        (Find-El "Wydanie" "Window") | Should Not BeNullOrEmpty
        Close-Dialog
    }

    It "rejects an over-stock OUT with a server error and records nothing" {
        Click-Point 450 212                       # select a data row
        $sel = Grid-Selected $g
        $before = Grid-Cell $g $sel 3             # its on-hand (On-hand column = 3)
        Click-Point 207 100                       # Wydaj (OUT)
        [void](Wait-Until { $null -ne (Find-El "Wydanie" "Window") })

        Send-Key "9999"                           # far more than any warehouse holds
        Send-Key "{ENTER}"                        # OK closes the dialog -> ExecuteMovement -> sp THROWs

        # The cleaned server error surfaces in a message box (exercises CWarehouseDoc's catch +
        # ShowError). The stored-proc text stays Polish (scope A), so match the body, not the
        # title (which equals the main window's).
        [void](Wait-Until { $null -ne (Find-El "wystarczaj") })
        (Find-El "wystarczaj") | Should Not BeNullOrEmpty

        Send-Key "{ENTER}"                        # dismiss the error box
        [void](Wait-Until { $null -eq (Find-El "wystarczaj") })
        (Grid-Cell $g $sel 3) | Should Be $before # transaction rolled back: on-hand unchanged
    }

    It "hides and restores a dockable pane; the centre grid reflows" {
        $w0 = Grid-Width $g
        Click-Point 178 50                        # Widok tab
        Click-Point 110 110                       # toggle Pulpit (left pane) off
        [void](Wait-Until { (Grid-Width $g) -ne $w0 })
        ((Grid-Width $g) -gt $w0) | Should Be $true   # grid widened into the freed space
        Click-Point 110 110                       # restore
        [void](Wait-Until { (Grid-Width $g) -eq $w0 })
        ((Grid-Width $g) -eq $w0) | Should Be $true
        Click-Point 104 50                        # back to Magazyn
    }

    It "F1 opens the About dialog" {
        Send-Key "{F1}"
        $find = { $a = Find-El "O programie" "Window"; if (-not $a) { $a = Find-El "About" "Window" }; $a }
        [void](Wait-Until { $null -ne (& $find) })
        (& $find) | Should Not BeNullOrEmpty
        Send-Key "{ENTER}"                        # close (OK)
        [void](Wait-Until { $null -eq (& $find) })
    }

    It "survives an extreme resize without crashing" {
        $h = Get-AppHwnd
        [void][Native]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 720, 480, 0x0050)
        Start-Sleep -Milliseconds 400
        (Get-Process app -ErrorAction SilentlyContinue) | Should Not BeNullOrEmpty
        $r = New-Object Native+RECT; [void][Native]::GetWindowRect($h, [ref]$r)
        (($r.Right - $r.Left) -eq 720) | Should Be $true
        ((Get-MainGrid) -ne [IntPtr]::Zero) | Should Be $true   # controls still alive, no crash
        Pin-App
    }
}
