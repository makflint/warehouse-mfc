@echo off
REM Build the microphone/STT self-test. Uses the SAME app/MicCapture (WASAPI) and
REM app/Stt (whisper) the app uses, so it reproduces the voice path on the console.
REM Run from the repo root:  tools\build_voice_probe.cmd  then  voice_probe.exe
REM (speak when it says RECORDING). Requires the whisper static libs already built
REM (see TODO.md "Build / test") and models\ggml-base.bin present.

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /MD /O2 ^
  /I app /I third_party\whisper.cpp\include /I third_party\whisper.cpp\ggml\include ^
  tools\voice_probe.cpp app\MicCapture.cpp app\Stt.cpp /Fe:voice_probe.exe ^
  /link /LIBPATH:third_party\whisper.cpp\build\src\Release ^
  /LIBPATH:third_party\whisper.cpp\build\ggml\src\Release ^
  whisper.lib ggml.lib ggml-cpu.lib ggml-base.lib advapi32.lib
