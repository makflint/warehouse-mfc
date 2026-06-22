# E2E voice test for the STT + parser pipeline. Runs WAVs through the REAL app code
# (wav_command.exe = Stt small+beam + core parseVoiceCommand) and asserts the command.
# Two corpora:
#   - real    : your recordings in tests\voice_fixtures\ (manifest expected.csv) = the
#               truthful accuracy measure.
#   - paulina : SAPI-synthesised phrases treated as clean GROUND-TRUTH input. A FAIL
#               here is a real STT/parser failure, flagged like any other (no excuse).
# Usage:  powershell -ExecutionPolicy Bypass -File tools\e2e_voice.ps1 [-Corpus both|real|paulina]
param([ValidateSet('both','real','paulina')] [string]$Corpus = 'both')
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# --- ensure the runner tool exists (self-build) -----------------------------------
if (-not (Test-Path '.\wav_command.exe')) {
    Write-Host 'Building wav_command.exe...' -ForegroundColor DarkGray
    $vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    $inc = '/I app /I core\include /I third_party\whisper.cpp\include /I third_party\whisper.cpp\ggml\include'
    $libs = '/link /LIBPATH:x64\Release /LIBPATH:third_party\whisper.cpp\build\src\Release ' +
            '/LIBPATH:third_party\whisper.cpp\build\ggml\src\Release ' +
            'core.lib whisper.lib ggml.lib ggml-cpu.lib ggml-base.lib advapi32.lib'
    cmd /c "`"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /MD /O2 $inc tools\wav_command.cpp app\Stt.cpp /Fe:wav_command.exe $libs" | Out-Null
    if (-not (Test-Path '.\wav_command.exe')) {
        Write-Host 'Build failed. Ensure whisper static libs + x64\Release\core.lib exist (see TODO.md).' -ForegroundColor Red
        return
    }
}

# --- run a corpus: cases = @{ id; file; kind; type?; qty?; sku? } ------------------
function Invoke-Corpus($label, $cases) {
    if (-not $cases) { return @{ pass = 0; fail = 0 } }
    $out = & .\wav_command.exe ($cases | ForEach-Object { $_.file }) 2>$null
    $pass = 0; $fail = 0
    Write-Host ''
    Write-Host "=== $label ===" -ForegroundColor Cyan
    foreach ($c in $cases) {
        $line = $out | Where-Object { $_ -match [regex]::Escape($c.file) } | Select-Object -First 1
        $gotKind = if ($line -match '->\s*(\w+)') { $matches[1] } else { '?' }
        $heard = if ($line -match 'heard=\[(.*?)\]') { $matches[1].Trim() } else { '' }
        $ok = ($gotKind -eq $c.kind)
        $detail = ''
        if ($c.kind -eq 'RecordMovement') {
            $gt = ''; $gq = ''; $gs = ''
            if ($line -match '\((IN|OUT) qty=(\d+) sku=(\w+)\)') { $gt = $matches[1]; $gq = [int]$matches[2]; $gs = $matches[3] }
            $ok = $ok -and ($gt -eq $c.type) -and ($gq -eq [int]$c.qty) -and ($gs -eq $c.sku)
            $detail = " [$gt qty=$gq sku=$gs]"
        }
        if ($ok) { $pass++ } else { $fail++ }
        $tag = if ($ok) { 'PASS' } else { 'FAIL' }
        Write-Host ("{0}  {1,-10} expect={2,-14} got={3}{4}" -f $tag, $c.id, $c.kind, $gotKind, $detail) -ForegroundColor ($(if ($ok) { 'Green' } else { 'Red' }))
        Write-Host ("           heard: `"$heard`"") -ForegroundColor DarkGray
    }
    Write-Host ("-- ${label}: $pass passed, $fail failed --") -ForegroundColor ($(if ($fail) { 'Yellow' } else { 'Green' }))
    return @{ pass = $pass; fail = $fail }
}

# --- corpus builders ---------------------------------------------------------------
function Get-RealCases {
    $csv = 'tests\voice_fixtures\expected.csv'
    if (-not (Test-Path $csv)) { Write-Host '(no real fixtures: tests\voice_fixtures\expected.csv)' -ForegroundColor DarkGray; return @() }
    # Skip rows whose WAV isn't present (recordings may be kept local / not cloned).
    $cases = Import-Csv $csv | Where-Object { Test-Path "tests\voice_fixtures\$($_.file)" } | ForEach-Object {
        @{ id = $_.file; file = "tests\voice_fixtures\$($_.file)"; kind = $_.kind; type = $_.type; qty = $_.qty; sku = $_.sku }
    }
    if (-not $cases) { Write-Host '(real fixtures listed but no WAV files present - skipping)' -ForegroundColor DarkGray }
    $cases
}

function Get-PaulinaCases {
    Add-Type -AssemblyName System.Speech
    $S=[char]0x015B; $Z=[char]0x017C; $O=[char]0x00F3; $E=[char]0x0119; $C=[char]0x0107
    $spec = @(
        @{ id='odswiez';  say="od${S}wie${Z}";                                       kind='Refresh' }
        @{ id='pokaz';    say="poka${Z} niskie stany";                               kind='ShowLowStock' }
        @{ id='anuluj';   say='anuluj';                                              kind='Undo' }
        @{ id='ponow';    say="pon${O}w";                                            kind='Redo' }
        @{ id='przyjmij'; say="przyjmij dziesi${E}${C} cztery pi${E}${C} dwa jeden"; kind='RecordMovement'; type='IN';  qty='10'; sku='4521' }
        @{ id='wydaj';    say="wydaj pi${E}${C} cztery pi${E}${C} dwa trzy";         kind='RecordMovement'; type='OUT'; qty='5';  sku='4523' }
    )
    $voice = New-Object System.Speech.Synthesis.SpeechSynthesizer
    $pl = $voice.GetInstalledVoices() | Where-Object { $_.VoiceInfo.Culture -like 'pl*' } | Select-Object -First 1
    if ($pl) { $voice.SelectVoice($pl.VoiceInfo.Name) }
    $fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(16000,
        [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
    foreach ($c in $spec) { $c.file = "e2e_$($c.id).wav"; $voice.SetOutputToWaveFile($c.file); $voice.Speak($c.say) }
    $voice.SetOutputToDefaultAudioDevice(); $voice.Dispose()
    return $spec
}

# --- run ---------------------------------------------------------------------------
$total = @{ pass = 0; fail = 0 }
if ($Corpus -in 'real', 'both')    { $r = Invoke-Corpus 'Real recordings (your voice)' (Get-RealCases); $total.pass += $r.pass; $total.fail += $r.fail }
if ($Corpus -in 'paulina', 'both') { $r = Invoke-Corpus 'Paulina (synthetic TTS)'      (Get-PaulinaCases); $total.pass += $r.pass; $total.fail += $r.fail }
Get-ChildItem e2e_*.wav -EA SilentlyContinue | Remove-Item -Force -EA SilentlyContinue

Write-Host ''
Write-Host ("TOTAL: $($total.pass) passed, $($total.fail) failed") -ForegroundColor ($(if ($total.fail) { 'Yellow' } else { 'Green' }))
exit $total.fail
