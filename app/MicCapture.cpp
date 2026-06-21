#include "MicCapture.h"

#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace mic {
namespace {

constexpr int kBitsPerSample = 16;
constexpr float kInt16Scale = 32768.0f;

WAVEFORMATEX monoFormat() {
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = kSampleRate;
    format.wBitsPerSample = kBitsPerSample;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    return format;
}

}  // namespace

std::vector<float> Record(int seconds) {
    if (seconds <= 0) {
        return {};
    }

    WAVEFORMATEX format = monoFormat();
    HANDLE filled = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (filled == nullptr) {
        return {};
    }

    HWAVEIN device = nullptr;
    if (waveInOpen(&device, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(filled), 0,
                   CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        CloseHandle(filled);
        return {};
    }

    // One buffer sized to the whole window; it is returned once full.
    std::vector<short> samples(static_cast<std::size_t>(kSampleRate) * seconds);
    WAVEHDR header{};
    header.lpData = reinterpret_cast<LPSTR>(samples.data());
    header.dwBufferLength = static_cast<DWORD>(samples.size() * sizeof(short));

    std::vector<float> out;
    if (waveInPrepareHeader(device, &header, sizeof(header)) == MMSYSERR_NOERROR &&
        waveInAddBuffer(device, &header, sizeof(header)) == MMSYSERR_NOERROR &&
        waveInStart(device) == MMSYSERR_NOERROR) {
        const DWORD limitMs = static_cast<DWORD>(seconds + 2) * 1000;
        DWORD waitedMs = 0;
        while ((header.dwFlags & WHDR_DONE) == 0 && waitedMs < limitMs) {
            WaitForSingleObject(filled, 200);
            waitedMs += 200;
        }
        waveInStop(device);

        const std::size_t got = header.dwBytesRecorded / sizeof(short);
        out.resize(got);
        for (std::size_t i = 0; i < got; ++i) {
            out[i] = samples[i] / kInt16Scale;
        }
    }

    waveInUnprepareHeader(device, &header, sizeof(header));
    waveInClose(device);
    CloseHandle(filled);
    return out;
}

}  // namespace mic
