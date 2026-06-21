#pragma once

#include <vector>

// Minimal microphone capture for the voice command path. Records a fixed window of
// mono 16 kHz audio from the default input device (the rate whisper expects) and
// returns it as normalised float samples in [-1, 1]. Empty vector on failure.
namespace mic {

constexpr int kSampleRate = 16000;

std::vector<float> Record(int seconds);

}  // namespace mic
