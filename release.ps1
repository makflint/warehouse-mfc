<#
  One command, from zero: build (Debug + Release) -> run the test gates -> build the installer.
  Produces installer\Output\WarehouseMFC-Setup.exe. Does NOT publish anywhere (push a GitHub
  release by hand with `gh` when you want one).

  Usage:
    powershell -File release.ps1            # build + test + installer
    powershell -File release.ps1 -NoSweep   # same, but skip the slow visual sweeps

  Stops (no installer) if any test gate fails. Needs Inno Setup 6 (ISCC) and the bundled
  assets under installer\assets\ (vc_redist.x64.exe, SqlLocalDB.msi — gitignored).
#>
param([switch]$NoSweep)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# 1) build (Debug + Release) and run the gates — reuse the local-CI runner.
& "$root\run-tests.ps1" -Build -NoSweep:$NoSweep
if ($LASTEXITCODE -ne 0) { throw "Test gates failed ($LASTEXITCODE) — installer not built." }

# 2) build the installer from the Release app.
$iscc = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) { throw "ISCC.exe not found at $iscc — install Inno Setup 6." }
foreach ($asset in 'vc_redist.x64.exe', 'SqlLocalDB.msi') {
    if (-not (Test-Path "$root\installer\assets\$asset")) {
        throw "Missing installer\assets\$asset (gitignored) — add it before building the installer."
    }
}
Write-Host "`n== installer ==" -ForegroundColor Cyan
& $iscc "$root\installer\warehouse-mfc.iss"
if ($LASTEXITCODE -ne 0) { throw "ISCC failed ($LASTEXITCODE)." }

Write-Host "`nDone -> installer\Output\WarehouseMFC-Setup.exe" -ForegroundColor Green
