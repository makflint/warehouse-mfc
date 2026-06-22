// Diagnostic probe: records with the SAME mic::Record (WASAPI) the app uses, reports
// captured samples + peak level, then runs the SAME Stt (whisper) wrapper. Splits
// "mic captured nothing" from "whisper recognised nothing". Run from the repo root.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "MicCapture.h"
#include "Stt.h"

int main() {
    std::printf("Loading model (models\\ggml-base.bin)...\n");
    Stt stt(L"models\\ggml-base.bin");
    std::printf("model loaded: %s\n", stt.ok() ? "YES" : "NO");

    std::printf("\n>>> RECORDING 5 s via WASAPI - speak now <<<\n");
    const std::vector<float> pcm = mic::Record(5);

    float peak = 0.0f;
    for (const float s : pcm) peak = std::fabs(s) > peak ? std::fabs(s) : peak;
    std::printf("\ncaptured samples = %zu  (%.2f s @ %d Hz)   peak = %.4f\n", pcm.size(),
                pcm.size() / static_cast<double>(mic::kSampleRate), mic::kSampleRate, peak);
    std::printf(peak < 0.01f ? "  -> SILENCE (no signal)\n" : "  -> SIGNAL PRESENT\n");

    if (stt.ok()) {
        std::printf("RECOGNISED = [%s]\n", stt.Transcribe(pcm).c_str());
    }
    return 0;
}
