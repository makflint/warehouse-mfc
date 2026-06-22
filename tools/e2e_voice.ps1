# E2E voice test (Paulina TTS): for each command, synthesize a 16 kHz mono WAV, run
# it through the REAL app pipeline (wav_command.exe = Stt small+beam + core
# parseVoiceCommand), and assert the resulting command matches expectations.
#
# NOTE: Paulina is a SYNTHETIC voice — useful as a stable fixture, but whisper reacts
# differently to it than to a real human, so this measures pipeline wiring, not real
# accuracy. Use real recordings for that. Run from the repo root:
#   powershell -ExecutionPolicy Bypass -File tools\e2e_voice.ps1
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Speech

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

$S=[char]0x015B; $Z=[char]0x017C; $O=[char]0x00F3; $E=[char]0x0119; $C=[char]0x0107

$cases = @(
    @{ id='odswiez';  say="od${S}wie${Z}";                                       kind='Refresh' }
    @{ id='pokaz';    say="poka${Z} niskie stany";                               kind='ShowLowStock' }
    @{ id='anuluj';   say='anuluj';                                              kind='Undo' }
    @{ id='ponow';    say="pon${O}w";                                            kind='Redo' }
    @{ id='przyjmij'; say="przyjmij dziesi${E}${C} cztery pi${E}${C} dwa jeden"; kind='RecordMovement'; type='IN';  qty=10; sku='4521' }
    @{ id='wydaj';    say="wydaj pi${E}${C} cztery pi${E}${C} dwa trzy";         kind='RecordMovement'; type='OUT'; qty=5;  sku='4523' }
)

# 1) Synthesize each phrase to a 16 kHz mono WAV.
$voice = New-Object System.Speech.Synthesis.SpeechSynthesizer
$pl = $voice.GetInstalledVoices() | Where-Object { $_.VoiceInfo.Culture -like 'pl*' } | Select-Object -First 1
if ($pl) { $voice.SelectVoice($pl.VoiceInfo.Name) }
$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(16000,
    [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
foreach ($c in $cases) {
    $c.file = "e2e_$($c.id).wav"
    $voice.SetOutputToWaveFile($c.file)
    $voice.Speak($c.say)
}
$voice.SetOutputToDefaultAudioDevice()
$voice.Dispose()

# 2) Run them all through the real Stt + parser.
$files = $cases | ForEach-Object { $_.file }
$out = & .\wav_command.exe @files 2>$null

# 3) Compare each result to the expectation.
$pass = 0; $fail = 0
Write-Host ''
foreach ($c in $cases) {
    $line = $out | Where-Object { $_ -match [regex]::Escape($c.file) } | Select-Object -First 1
    $gotKind = if ($line -match '->\s*(\w+)') { $matches[1] } else { '?' }
    $heard = if ($line -match 'heard=\[(.*?)\]') { $matches[1].Trim() } else { '' }
    $ok = ($gotKind -eq $c.kind)
    $detail = ''
    if ($c.kind -eq 'RecordMovement') {
        $gt=''; $gq=''; $gs=''
        if ($line -match '\((IN|OUT) qty=(\d+) sku=(\w+)\)') { $gt=$matches[1]; $gq=[int]$matches[2]; $gs=$matches[3] }
        $ok = $ok -and ($gt -eq $c.type) -and ($gq -eq $c.qty) -and ($gs -eq $c.sku)
        $detail = " [$gt qty=$gq sku=$gs]"
    }
    if ($ok) { $pass++ } else { $fail++ }
    $tag = if ($ok) { 'PASS' } else { 'FAIL' }
    $col = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ("{0}  {1,-9} expect={2,-14} got={3}{4}" -f $tag, $c.id, $c.kind, $gotKind, $detail) -ForegroundColor $col
    Write-Host ("         heard: `"$heard`"") -ForegroundColor DarkGray
}

Write-Host ''
Write-Host ("RESULT: $pass passed, $fail failed (of $($cases.Count))") -ForegroundColor ($(if ($fail) { 'Yellow' } else { 'Green' }))
Write-Host '(Paulina is synthetic - a FAIL here often means whisper mis-hears the TTS, not a code bug.)' -ForegroundColor DarkGray
exit $fail
