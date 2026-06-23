# Assertion tests for the main stock grid, reading cell text cross-process (gridreader.ps1).
# Covers sweep steps 1-5 (sorting), 6 (selection), 7-8 (filter), 12-14 (on-hand after a
# movement + undo/redo). The sort *comparators* are unit-tested in core; here we assert the
# running grid actually reorders. The movement case records then undoes (net-zero), and a
# finally-block guarantees the seed state is restored even if an assertion fails.

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. "$here\..\manual\uia.ps1"
. "$here\gridreader.ps1"

# Column indices + header click points (window pixels), mirroring CStockView.
$COL_WH = 0; $COL_SKU = 1; $COL_PROD = 2; $COL_STAN = 3
$HdrMagazyn = @(340, 192); $HdrSymbol = @(420, 192); $HdrProdukt = @(565, 192); $HdrStan = @(710, 192)
function Tap($p) { Click-Point $p[0] $p[1]; Start-Sleep -Milliseconds 300 }

function Is-NonDecreasingInt($vals) {
    for ($i = 1; $i -lt $vals.Count; $i++) { if ([int]$vals[$i] -lt [int]$vals[$i - 1]) { return $false } }
    return $true
}
function Is-NonIncreasingInt($vals) {
    for ($i = 1; $i -lt $vals.Count; $i++) { if ([int]$vals[$i] -gt [int]$vals[$i - 1]) { return $false } }
    return $true
}
# Ordinal (byte-order) comparison to match the app's std::string::compare on UTF-8.
function Is-NonDecreasingText($vals) {
    for ($i = 1; $i -lt $vals.Count; $i++) { if ([string]::CompareOrdinal($vals[$i], $vals[$i - 1]) -lt 0) { return $false } }
    return $true
}

Describe "Stock grid" {

    Ensure-App; Send-Key "{ESC}"; Click-Point 104 50; Pin-App   # Magazyn tab
    $g = Get-MainGrid

    It "is populated" {
        ((Grid-Count $g) -gt 0) | Should Be $true
    }

    It "sorts Stan numerically: ascending then descending" {
        Tap $HdrStan
        (Is-NonDecreasingInt (Grid-Column $g $COL_STAN)) | Should Be $true
        Tap $HdrStan
        (Is-NonIncreasingInt (Grid-Column $g $COL_STAN)) | Should Be $true
    }

    It "sorts text columns lexicographically (Magazyn, Symbol, Produkt)" {
        Tap $HdrMagazyn; (Is-NonDecreasingText (Grid-Column $g $COL_WH))   | Should Be $true
        Tap $HdrSymbol;  (Is-NonDecreasingText (Grid-Column $g $COL_SKU))  | Should Be $true
        Tap $HdrProdukt; (Is-NonDecreasingText (Grid-Column $g $COL_PROD)) | Should Be $true
    }

    It "narrows to low-stock rows under the filter, then restores" {
        Tap $HdrMagazyn
        $all = Grid-Count $g
        Click-Point 102 100                                     # Tylko niskie ON
        [void](Wait-Until { (Grid-Count $g) -ne $all })
        $low = Grid-Count $g
        (($low -lt $all) -and ($low -gt 0)) | Should Be $true
        Click-Point 102 100                                     # OFF
        [void](Wait-Until { (Grid-Count $g) -eq $all })
        ((Grid-Count $g) -eq $all) | Should Be $true
    }

    It "tracks the selected row" {
        Click-Point 450 232
        ((Grid-Selected $g) -ge 0) | Should Be $true
    }

    It "double-clicking a row opens the record dialog" {
        Close-Dialog
        DblClick-Point 450 212
        [void](Wait-Until { Test-RecordDialogOpen })
        Test-RecordDialogOpen | Should Be $true
        Close-Dialog                       # cancel — nothing recorded
        Test-RecordDialogOpen | Should Be $false
    }

    It "right-clicking a row shows a context menu" {
        Close-Dialog
        Click-Point 450 212 -Right
        [void](Wait-Until { Test-ContextMenuOpen })
        Test-ContextMenuOpen | Should Be $true
        Send-Key "{ESC}"                   # dismiss
        [void](Wait-Until { -not (Test-ContextMenuOpen) })
        Test-ContextMenuOpen | Should Be $false
    }

    It "applies a movement to on-hand and undo/redo restores it" {
        Click-Point 450 212                       # select a row
        $sel = Grid-Selected $g
        $wh = Grid-Cell $g $sel $COL_WH
        $sku = Grid-Cell $g $sel $COL_SKU
        $before = [int](Grid-Cell $g $sel $COL_STAN)
        # Read the affected row's on-hand by content (its position changes after the re-sort).
        $stanOf = { $r = Grid-FindRow $g $COL_WH $wh $COL_SKU $sku; if ($r -lt 0) { -1 } else { [int](Grid-Cell $g $r $COL_STAN) } }
        try {
            Click-Point 158 108; Send-Key "{ENTER}"                       # Przyjmij, qty 1 -> +1
            [void](Wait-Until { (& $stanOf) -ne $before })
            (& $stanOf) | Should Be ($before + 1)
            Send-Key "^z"                                                 # undo
            [void](Wait-Until { (& $stanOf) -ne ($before + 1) })
            (& $stanOf) | Should Be $before
            Send-Key "^y"                                                 # redo
            [void](Wait-Until { (& $stanOf) -ne $before })
            (& $stanOf) | Should Be ($before + 1)
        } finally {
            # Restore the seed state no matter where an assertion stopped: undo our own +1s.
            for ($k = 0; $k -lt 3; $k++) {
                if ((& $stanOf) -le $before) { break }
                Send-Key "^z"; [void](Wait-Until { (& $stanOf) -le $before } 2000)
            }
        }
    }
}
