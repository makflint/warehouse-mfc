# Assertion-based UI tests for the record-movement dialog (Pester 3.x, built into Windows).
# Drives the *running* Release app via UI Automation and asserts on control state — no human
# screenshot review. Every case cancels out of the dialog, so nothing is recorded: the demo
# DB is left unchanged and the suite is repeatable.
#
# Run:  powershell -File tests\ui\Run.ps1   (or: Invoke-Pester tests\ui)

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. "$here\..\manual\uia.ps1"

Describe "Record-movement dialog" {

    Ensure-App

    It "pre-selects the selected grid row's product/warehouse, and follows a different row" {
        # Top row (selected on startup).
        Close-Dialog
        Select-GridRow 212
        Open-RecordIn
        Test-RecordDialogOpen | Should Be $true
        $pTop = Read-DlgField $IdcProduct
        $wTop = Read-DlgField $IdcWarehouse
        Close-Dialog

        # A different row (jump to the last item — a different warehouse, grid is sorted by Magazyn).
        Select-GridRow 212
        Send-Key "{END}"
        Open-RecordIn
        $pBot = Read-DlgField $IdcProduct
        $wBot = Read-DlgField $IdcWarehouse
        Close-Dialog

        # Sanity: both reads are real warehouse codes.
        $wTop | Should Match '^MAG-'
        $wBot | Should Match '^MAG-'
        # The fix: the combos reflect the grid selection, not a fixed default. If the dialog
        # still hard-coded the first product/warehouse, these two would be identical.
        "$pTop|$wTop" | Should Not Be "$pBot|$wBot"
    }

    It "rejects an out-of-range quantity and keeps the dialog open" {
        Select-GridRow 212
        Open-RecordIn
        Test-RecordDialogOpen | Should Be $true

        Send-Key "0"          # quantity 0 is below the minimum (1)
        Send-Key "{ENTER}"    # OK -> manual Polish validation should reject
        Send-Key "{ENTER}"    # dismiss the message box

        # Still open: validation refused to accept, nothing was recorded.
        Test-RecordDialogOpen | Should Be $true

        Close-Dialog
        Test-RecordDialogOpen | Should Be $false
    }
}
