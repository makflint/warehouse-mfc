#include "Stt.h"

#define NOMINMAX  // keep std::min/std::max usable after <windows.h>
#include <windows.h>

#include <algorithm>
#include <thread>

#include "whisper.h"

namespace {

constexpr int kMaxThreads = 4;  // base model on a few short words needs no more

std::string narrowUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), out.data(), size,
                        nullptr, nullptr);
    return out;
}

int recognitionThreads() {
    const unsigned hw = std::thread::hardware_concurrency();
    return std::max(1, std::min(kMaxThreads, static_cast<int>(hw)));
}

}  // namespace

Stt::Stt(const std::wstring& modelPath) {
    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = false;
    ctx_ = whisper_init_from_file_with_params(narrowUtf8(modelPath).c_str(), cparams);
}

Stt::~Stt() {
    if (ctx_ != nullptr) {
        whisper_free(ctx_);
    }
}

std::string Stt::Transcribe(const std::vector<float>& pcm16k) const {
    if (ctx_ == nullptr || pcm16k.empty()) {
        return {};
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.language = "pl";
    params.translate = false;
    params.no_timestamps = true;
    params.single_segment = true;  // one short command per utterance
    params.print_progress = false;
    params.print_realtime = false;
    params.print_special = false;
    params.print_timestamps = false;
    params.n_threads = recognitionThreads();

    if (whisper_full(ctx_, params, pcm16k.data(), static_cast<int>(pcm16k.size())) != 0) {
        return {};
    }

    std::string text;
    const int segments = whisper_full_n_segments(ctx_);
    for (int i = 0; i < segments; ++i) {
        text += whisper_full_get_segment_text(ctx_, i);
    }
    return text;
}
