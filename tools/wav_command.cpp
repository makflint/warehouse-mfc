// End-to-end check of the app's voice LOGIC on a WAV: read it, run the SAME Stt
// (whisper small + beam) and the SAME core parseVoiceCommand the app uses, and print
// the recognised text + resulting command.   wav_command.exe <in.wav> [in2.wav ...]

#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Stt.h"
#include "warehouse/voice_command_parser.hpp"

namespace {
std::vector<float> readWav(const char* path) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return {};
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> raw(n > 0 ? n : 0);
    std::fread(raw.data(), 1, raw.size(), f);
    std::fclose(f);
    std::size_t i = 12;
    while (i + 8 <= raw.size()) {
        uint32_t sz;
        std::memcpy(&sz, &raw[i + 4], 4);
        if (std::memcmp(&raw[i], "data", 4) == 0) {
            const int16_t* s = reinterpret_cast<const int16_t*>(&raw[i + 8]);
            const std::size_t count = sz / 2;
            std::vector<float> pcm(count);
            for (std::size_t k = 0; k < count; ++k) pcm[k] = s[k] / 32768.0f;
            return pcm;
        }
        i += 8 + sz + (sz & 1);
    }
    return {};
}

const char* kindName(warehouse::CommandKind k) {
    using K = warehouse::CommandKind;
    switch (k) {
        case K::RecordMovement: return "RecordMovement";
        case K::ShowLowStock:   return "ShowLowStock";
        case K::Refresh:        return "Refresh";
        case K::Undo:           return "Undo";
        case K::Redo:           return "Redo";
        default:                return "Unknown";
    }
}

}  // namespace

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);  // so printf of UTF-8 text renders and is pipe-captured

    std::wstring model = L"models\\ggml-small.bin";
    std::vector<const char*> wavs;
    for (int a = 1; a < argc; ++a) {
        if (std::strcmp(argv[a], "--model") == 0 && a + 1 < argc) {
            const char* m = argv[++a];
            model.assign(m, m + std::strlen(m));  // model paths are ASCII
        } else {
            wavs.push_back(argv[a]);
        }
    }

    Stt stt(model);
    if (!stt.ok()) { std::printf("model load failed\n"); return 1; }

    for (const char* wav : wavs) {
        const std::vector<float> pcm = readWav(wav);
        const std::string text = stt.Transcribe(pcm);
        const warehouse::ParsedCommand c = warehouse::parseVoiceCommand(text);
        std::printf("%-22s heard=[%s]  -> %s", wav, text.c_str(), kindName(c.kind));
        if (c.kind == warehouse::CommandKind::RecordMovement) {
            std::printf(" (%s qty=%d sku=%s)",
                        c.type == warehouse::MovementType::In ? "IN" : "OUT", c.qty, c.sku.c_str());
        }
        std::printf("\n");
    }
    return 0;
}
