# Guided recorder: shows each command to say, then voice_probe counts down (3-2-1)
# and records 5 s to a WAV. Run from the repo root:
#   powershell -ExecutionPolicy Bypass -File tools\record_commands.ps1
# ASCII-only source (PowerShell 5.1 reads .ps1 as ANSI); Polish phrases are built
# from Unicode code points so they cannot mojibake.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$chS = [char]0x015B  # s-acute
$chZ = [char]0x017C  # z-dot
$chO = [char]0x00F3  # o-acute
$chE = [char]0x0119  # e-ogonek
$chC = [char]0x0107  # c-acute

$commands = @(
    @{ file = 'odswiez.wav';  say = "od${chS}wie${chZ}" },                                   # odswiez
    @{ file = 'pokaz.wav';    say = "poka${chZ} niskie stany" },                             # pokaz niskie stany
    @{ file = 'cofnij.wav';   say = 'cofnij' },
    @{ file = 'ponow.wav';    say = "pon${chO}w" },                                          # ponow
    @{ file = 'przyjmij.wav'; say = "przyjmij dziesi${chE}${chC} sztuk, cztery pi${chE}${chC} dwa jeden" }
)

if (-not (Test-Path '.\voice_probe.exe')) {
    Write-Host 'Brak voice_probe.exe - uruchom z katalogu projektu (cd ...\warehouse-mfc).' -ForegroundColor Red
    return
}

$i = 1
foreach ($cmd in $commands) {
    Write-Host ''
    Write-Host '==================================================' -ForegroundColor Cyan
    Write-Host ("  [$i/5]  POWIEDZ TO:") -ForegroundColor Cyan
    Write-Host ("      >>>  " + $cmd.say + "  <<<") -ForegroundColor Yellow
    Write-Host '==================================================' -ForegroundColor Cyan
    Write-Host '(Enter -> odlicza 3-2-1 -> mow)' -ForegroundColor DarkGray
    [void](Read-Host)
    & .\voice_probe.exe $cmd.file $cmd.say
    $i++
}

Write-Host ''
Write-Host 'Gotowe. Pliki WAV sa w katalogu projektu. Napisz Claude: done' -ForegroundColor Green
