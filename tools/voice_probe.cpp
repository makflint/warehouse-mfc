// Records 5 s with the SAME mic::Record (WASAPI) the app uses, SAVES it to a 16 kHz
// mono WAV (so we can tune recognition offline on the exact audio), and prints what
// whisper recognises. It shows the phrase to say and counts down 3-2-1 first.
//   voice_probe.exe [out.wav] [phrase words...]
// e.g.  voice_probe.exe odswiez.wav odśwież

#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "MicCapture.h"
#include "Stt.h"

namespace {

// Print a UTF-16 line to the console so Polish letters render regardless of codepage.
void consoleLine(const std::wstring& text) {
    DWORD written = 0;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), text.c_str(),
                  static_cast<DWORD>(text.size()), &written, nullptr);
}

void writeWav(const std::string& path, const std::vector<float>& pcm, int rate) {
    std::vector<int16_t> s(pcm.size());
    for (std::size_t i = 0; i < pcm.size(); ++i) {
        float v = pcm[i];
        v = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
        s[i] = static_cast<int16_t>(v * 32767.0f);
    }
    const uint32_t dataBytes = static_cast<uint32_t>(s.size() * sizeof(int16_t));
    const uint32_t byteRate = static_cast<uint32_t>(rate) * 2;
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + dataBytes); std::fwrite("WAVE", 1, 4, f);
    std::fwrite("fmt ", 1, 4, f); u32(16); u16(1); u16(1);
    u32(static_cast<uint32_t>(rate)); u32(byteRate); u16(2); u16(16);
    std::fwrite("data", 1, 4, f); u32(dataBytes);
    std::fwrite(s.data(), 1, dataBytes, f);
    std::fclose(f);
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    const std::wstring outW = (argc > 1) ? argv[1] : L"voice_capture.wav";
    const std::string out(outW.begin(), outW.end());  // filename is ASCII
    std::wstring phrase;
    for (int i = 2; i < argc; ++i) {
        if (!phrase.empty()) phrase += L" ";
        phrase += argv[i];
    }

    std::printf("Loading model (models\\ggml-base.bin)...\n");
    Stt stt(L"models\\ggml-base.bin");
    std::printf("model loaded: %s\n", stt.ok() ? "YES" : "NO");

    if (!phrase.empty()) {
        consoleLine(L"\n==================================================\n");
        consoleLine(L"   MASZ POWIEDZIEĆ:   " + phrase + L"\n");
        consoleLine(L"==================================================\n");
    }
    std::printf("\nPrzygotuj sie...\n");
    for (int t = 3; t >= 1; --t) {
        std::printf("   %d...\n", t);
        std::fflush(stdout);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (!phrase.empty()) {
        consoleLine(L"\n>>> MÓW TERAZ:   " + phrase + L"   (5 s) <<<\n");
    } else {
        std::printf("\n>>> MOW TERAZ (5 s) <<<\n");
    }
    std::fflush(stdout);

    const std::vector<float> pcm = mic::Record(5);
    std::printf("(koniec nagrania)\n");

    float peak = 0.0f;
    for (const float v : pcm) peak = std::fabs(v) > peak ? std::fabs(v) : peak;
    std::printf("captured samples=%zu peak=%.3f\n", pcm.size(), peak);

    writeWav(out, pcm, mic::kSampleRate);
    std::printf("saved WAV: %s\n", out.c_str());

    if (stt.ok()) std::printf("RECOGNISED = [%s]\n", stt.Transcribe(pcm).c_str());
    return 0;
}
