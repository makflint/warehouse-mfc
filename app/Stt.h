#pragma once

#include <string>
#include <vector>

struct whisper_context;

// Offline Polish speech-to-text via whisper.cpp. The ggml model is loaded once at
// construction and reused; Transcribe() turns 16 kHz mono PCM into recognised text.
// No networking — inference is fully local. ok() reports whether the model loaded.
class Stt {
public:
    explicit Stt(const std::wstring& modelPath);
    ~Stt();
    Stt(const Stt&) = delete;
    Stt& operator=(const Stt&) = delete;

    bool ok() const { return ctx_ != nullptr; }

    // Recognise Polish speech in normalised float PCM ([-1, 1], 16 kHz mono).
    // Returns UTF-8 text (empty on failure or silence).
    std::string Transcribe(const std::vector<float>& pcm16k) const;

private:
    whisper_context* ctx_ = nullptr;
};
