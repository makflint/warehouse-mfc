<#
  Line coverage via OpenCppCoverage.

  Default (core only, Debug):  the pure-C++ TDD layer, accurate (Release folds/inlines lines, which
  distorts coverage — so the depth metric uses Debug). data/ and app/ aren't unit-tested.

  -All (whole codebase, Release):  breadth picture across core/ + data/ + app/. Runs every driver
  under coverage and merges them — core_tests (unit), data_smoke (integration), and the running app
  driven by BOTH the assertion suite (tests/ui) AND the visual sweep (which switches theme, opens
  menus/About/panes — paths the assertion suite never hits). Release is used so the app runs
  standalone; optimisation makes this a *floor*, not an exact number.

  Usage:
    powershell -File tools\coverage.ps1                 # core only, Debug, accurate
    powershell -File tools\coverage.ps1 -All            # whole codebase, Release, merged
    powershell -File tools\coverage.ps1 -All -NoBuild   # skip the build, measure existing exes
    powershell -File tools\coverage.ps1 -Min 90         # also fail (exit 1) below 90%

  Needs OpenCppCoverage (winget install OpenCppCoverage.OpenCppCoverage). -All also needs an
  interactive desktop + LocalDB (it drives the real app, like tests/ui).
#>
param(
    [switch]$NoBuild,
    [switch]$All,
    [double]$Min = 0   # 0 = report only; >0 = gate on this percentage
)

$root = Split-Path $PSScriptRoot -Parent
$cov  = "$root\coverage"

$occ = Get-Command OpenCppCoverage -ErrorAction SilentlyContinue
if (-not $occ) { $occ = Get-Item "${env:ProgramFiles}\OpenCppCoverage\OpenCppCoverage.exe" -ErrorAction SilentlyContinue }
if (-not $occ) {
    Write-Host "OpenCppCoverage not found - install it (winget install OpenCppCoverage.OpenCppCoverage)." -ForegroundColor Yellow
    exit 2
}
$occ = if ($occ.Source) { $occ.Source } else { $occ.FullName }

function Find-MsBuild {
    $m = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path $m)) { $m = (Get-Command msbuild -ErrorAction SilentlyContinue).Source }
    if (-not $m) { throw "msbuild not found - open a Developer prompt or install VS 2022." }
    return $m
}
function Reset-Db {
    $pipe = (sqllocaldb info MSSQLLocalDB | Select-String 'Instance pipe name').ToString().Split(':',2)[1].Trim()
    sqlcmd -S $pipe -d Warehouse -i "$root\db\02_seed.sql" | Out-Null
}
function Report($title) {
    [xml]$x = Get-Content "$cov\cobertura.xml"
    $cls = $x.coverage.packages.package.classes.class
    # The merged report can list a file once per module it was linked into; dedupe by filename.
    $files = $cls | Group-Object filename | ForEach-Object {
        $c = $_.Group | Sort-Object { ($_.lines.line | Where-Object {[int]$_.hits -gt 0}).Count } -Descending | Select-Object -First 1
        $valid = ($c.lines.line | Measure-Object).Count
        $covd  = ($c.lines.line | Where-Object {[int]$_.hits -gt 0} | Measure-Object).Count
        $layer = if ($_.Name -match '\\(core|data|app)\\') { $matches[1] } elseif ($_.Name -match '\\(core|data|app)\.') { $matches[1] } else { 'other' }
        [pscustomobject]@{ File=($_.Name -replace '.*warehouse-mfc\\',''); Layer=$layer; Valid=$valid; Covered=$covd }
    } | Where-Object { $_.Layer -ne 'other' }

    Write-Host "`n================ $title ================" -ForegroundColor Cyan
    foreach ($g in $files | Group-Object Layer | Sort-Object Name) {
        $v = ($g.Group | Measure-Object Valid -Sum).Sum; $c = ($g.Group | Measure-Object Covered -Sum).Sum
        Write-Host ("  {0,-5} {1,6:N1}%  ({2}/{3} lines)" -f $g.Name, (100*$c/$v), $c, $v)
    }
    $pct = 100*[double]$x.coverage.'line-rate'
    Write-Host ("  ----") -ForegroundColor DarkGray
    Write-Host ("  {0,-5} {1,6:N1}%  ({2}/{3} lines)" -f 'TOTAL', $pct, $x.coverage.'lines-covered', $x.coverage.'lines-valid') `
        -ForegroundColor $(if ($Min -gt 0 -and $pct -lt $Min) { 'Red' } else { 'Green' })
    Write-Host "  --- per file ---" -ForegroundColor DarkGray
    $files | Sort-Object File | ForEach-Object { Write-Host ("  {0,6:N0}%  {1}" -f (100*$_.Covered/[math]::Max(1,$_.Valid)), $_.File) }
    Write-Host ("  report: coverage\html\index.html") -ForegroundColor DarkGray
    Write-Host "=========================================" -ForegroundColor Cyan
    if ($Min -gt 0 -and $pct -lt $Min) { Write-Host "Coverage $([math]::Round($pct,1))% below -Min $Min%." -ForegroundColor Red; exit 1 }
}

if (Test-Path $cov) { Remove-Item $cov -Recurse -Force }
New-Item -ItemType Directory -Path $cov -Force | Out-Null

# ---------------------------------------------------------------- core only (Debug, accurate)
if (-not $All) {
    if (-not $NoBuild) {
        Write-Host "== build Debug core_tests ==" -ForegroundColor Cyan
        & (Find-MsBuild) "$root\warehouse-mfc.sln" /p:Configuration=Debug /p:Platform=x64 /t:core_tests /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw "Debug core_tests build failed" }
    }
    Write-Host "== coverage: core_tests (Debug) ==" -ForegroundColor Cyan
    & $occ --sources "core\src" --sources "core\include" --modules "core_tests.exe" --quiet `
        --export_type "html:$cov\html" --export_type "cobertura:$cov\cobertura.xml" `
        --working_dir $root -- "$root\x64\Debug\core_tests.exe"
    Report "COVERAGE (core, Debug)"
    exit 0
}

# ---------------------------------------------------------------- whole codebase (-All, Release)
$cfg = 'Release'
if (-not $NoBuild) {
    Write-Host "== build $cfg (core_tests + data_smoke + app) ==" -ForegroundColor Cyan
    & (Find-MsBuild) "$root\warehouse-mfc.sln" /p:Configuration=$cfg /p:Platform=x64 /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "$cfg build failed" }
}

$src = @('--sources','core\src','--sources','core\include',
         '--sources','data\src','--sources','data\include','--sources','app')

Stop-Process -Name app -Force -ErrorAction SilentlyContinue
Reset-Db

Write-Host "`n== [1/3] coverage: core_tests (unit) ==" -ForegroundColor Cyan
& $occ @src --modules core_tests.exe --quiet --export_type "binary:$cov\core.cov" --working_dir $root -- "$root\x64\$cfg\core_tests.exe"

Write-Host "`n== [2/3] coverage: data_smoke (integration) ==" -ForegroundColor Cyan
& $occ @src --modules data_smoke.exe --quiet --export_type "binary:$cov\data.cov" --working_dir $root -- "$root\x64\$cfg\data_smoke.exe"

Write-Host "`n== [3/3] coverage: app driven by tests/ui + sweep ==" -ForegroundColor Cyan
Write-Host "   launching app under coverage..." -ForegroundColor DarkGray
$occArgs = $src + @('--modules','app.exe','--quiet','--export_type',"binary:$cov\app.cov",'--working_dir',$root,'--',"$root\app\x64\$cfg\app.exe")
$occProc = Start-Process $occ -ArgumentList $occArgs -PassThru
Start-Sleep -Seconds 6
Write-Host "   -> driving with tests/ui (assertion suite)..." -ForegroundColor DarkGray
& powershell -NoProfile -ExecutionPolicy Bypass -File "$root\tests\ui\Run.ps1"
Write-Host "   -> driving with sweep (theme/menus/About/panes)..." -ForegroundColor DarkGray
& powershell -NoProfile -ExecutionPolicy Bypass -File "$root\tests\manual\sweep.ps1"   # no -Lang: keeps the same instance
Stop-Process -Name app -Force -ErrorAction SilentlyContinue
if (-not $occProc.WaitForExit(60000)) { Write-Host "   OCC app run did not exit in time" -ForegroundColor Yellow }

Write-Host "`n== merge ==" -ForegroundColor Cyan
& $occ --input_coverage "$cov\core.cov" --input_coverage "$cov\data.cov" --input_coverage "$cov\app.cov" `
    --quiet --export_type "html:$cov\html" --export_type "cobertura:$cov\cobertura.xml"

Stop-Process -Name app -Force -ErrorAction SilentlyContinue
Reset-Db
Report "COVERAGE (whole codebase, Release)"
exit 0
