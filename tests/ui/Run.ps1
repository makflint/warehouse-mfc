# Run the assertion-based UI suite and exit with the number of failed tests (0 = green),
# so it can gate like any other test runner. Needs an interactive desktop + the Release
# app built (tests launch it if it isn't already running).
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$result = Invoke-Pester -Path $here -PassThru -Quiet
"UI tests: $($result.PassedCount) passed, $($result.FailedCount) failed"
exit $result.FailedCount
