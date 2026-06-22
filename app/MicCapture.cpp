#include "MicCapture.h"

#define NOMINMAX
#include <windows.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

#include <algorithm>
#include <cmath>
#include <vector>

#pragma comment(lib, "ole32.lib")

#ifdef MIC_DEBUG
#include <cstdio>
#define MICLOG(...) std::fprintf(stderr, "[mic] " __VA_ARGS__)
#else
#define MICLOG(...) ((void)0)
#endif

// Capture from the default microphone via WASAPI (shared mode). Legacy waveIn does
// not work on modern DMIC arrays (e.g. Intel Smart Sound), so we use Core Audio:
// grab the device's mix format, record for the window, then downmix to mono and
// resample to 16 kHz float — the format whisper expects.

namespace mic {
namespace {

constexpr int kPollMs = 10;

template <typename T>
void release(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

// One source frame (interleaved, srcChannels) -> a single mono float sample.
float frameToMono(const BYTE* data, std::size_t frame, int channels, bool isFloat,
                  int bytesPerSample) {
    float sum = 0.0f;
    for (int c = 0; c < channels; ++c) {
        const std::size_t i = frame * channels + c;
        if (isFloat) {
            sum += reinterpret_cast<const float*>(data)[i];
        } else if (bytesPerSample == 2) {
            sum += reinterpret_cast<const short*>(data)[i] / 32768.0f;
        } else if (bytesPerSample == 4) {
            sum += reinterpret_cast<const int*>(data)[i] / 2147483648.0f;
        }
    }
    return sum / channels;
}

}  // namespace

std::vector<float> Record(int seconds) {
    if (seconds <= 0) {
        return {};
    }

    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool ownCom = SUCCEEDED(comInit);  // RPC_E_CHANGED_MODE => COM already up
    MICLOG("CoInitializeEx hr=0x%08lX ownCom=%d\n", comInit, ownCom);

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* client = nullptr;
    IAudioCaptureClient* capture = nullptr;
    WAVEFORMATEX* mix = nullptr;
    std::vector<float> mono;
    HRESULT hr = S_OK;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          IID_PPV_ARGS(&enumerator));
    MICLOG("CoCreateInstance(enumerator) hr=0x%08lX\n", hr);
    if (FAILED(hr)) goto cleanup;

    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
    MICLOG("GetDefaultAudioEndpoint(eCapture) hr=0x%08lX\n", hr);
    if (FAILED(hr)) goto cleanup;

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                          reinterpret_cast<void**>(&client));
    MICLOG("Activate(IAudioClient) hr=0x%08lX\n", hr);
    if (FAILED(hr)) goto cleanup;

    hr = client->GetMixFormat(&mix);
    MICLOG("GetMixFormat hr=0x%08lX\n", hr);
    if (FAILED(hr)) goto cleanup;
    MICLOG("mix: tag=%u ch=%u rate=%lu bits=%u\n", mix->wFormatTag, mix->nChannels,
           mix->nSamplesPerSec, mix->wBitsPerSample);

    {
        // Shared mode: hnsBufferDuration must be 0 (the engine picks its own period);
        // we poll across the whole window and accumulate, so no large buffer is needed.
        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mix, nullptr);
        MICLOG("Initialize hr=0x%08lX\n", hr);
        if (FAILED(hr)) goto cleanup;

        hr = client->GetService(IID_PPV_ARGS(&capture));
        MICLOG("GetService(IAudioCaptureClient) hr=0x%08lX\n", hr);
        if (FAILED(hr)) goto cleanup;

        hr = client->Start();
        MICLOG("Start hr=0x%08lX\n", hr);
        if (FAILED(hr)) goto cleanup;

        const int srcChannels = mix->nChannels;
        const int srcRate = static_cast<int>(mix->nSamplesPerSec);
        const int bytesPerSample = mix->wBitsPerSample / 8;
        const bool isFloat =
            mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE && mix->wBitsPerSample == 32);

        mono.reserve(static_cast<std::size_t>(srcRate) * seconds);
        std::size_t packets = 0;
        const ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(seconds) * 1000;
        while (GetTickCount64() < deadline) {
            UINT32 available = 0;
            if (FAILED(capture->GetNextPacketSize(&available)) || available == 0) {
                Sleep(kPollMs);
                continue;
            }
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) {
                break;
            }
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
            for (UINT32 f = 0; f < frames; ++f) {
                mono.push_back(silent ? 0.0f
                                      : frameToMono(data, f, srcChannels, isFloat, bytesPerSample));
            }
            capture->ReleaseBuffer(frames);
            ++packets;
        }
        client->Stop();
        MICLOG("captured packets=%zu frames(mono)=%zu @ %d Hz\n", packets, mono.size(), srcRate);

        // Resample mono (srcRate) -> 16 kHz with linear interpolation.
        if (srcRate > 0 && !mono.empty()) {
            const double step = static_cast<double>(srcRate) / kSampleRate;
            std::vector<float> out;
            out.reserve(static_cast<std::size_t>(mono.size() / step) + 1);
            for (double pos = 0; pos + 1 < mono.size(); pos += step) {
                const std::size_t i = static_cast<std::size_t>(pos);
                const float frac = static_cast<float>(pos - i);
                out.push_back(mono[i] * (1.0f - frac) + mono[i + 1] * frac);
            }
            mono.swap(out);
        }

        // Normalise level: DMIC arrays are often quiet, which under-drives whisper.
        // Scale the peak toward ~0.5, capped so we never amplify near-silence/noise.
        float peak = 0.0f;
        for (const float s : mono) peak = std::max(peak, std::fabs(s));
        if (peak > 0.02f) {
            const float gain = std::min(0.5f / peak, 20.0f);
            if (gain > 1.0f) {
                for (float& s : mono) s *= gain;
            }
        }
    }

cleanup:
    if (mix != nullptr) {
        CoTaskMemFree(mix);
    }
    release(capture);
    release(client);
    release(device);
    release(enumerator);
    if (ownCom) {
        CoUninitialize();
    }
    return mono;
}

}  // namespace mic
