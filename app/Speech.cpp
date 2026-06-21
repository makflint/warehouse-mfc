#include "framework.h"

#include "Speech.h"

#include <sphelper.h>

namespace {
// pl-PL locale id (0x0415) as the hex string SAPI voice tokens use in "Language=".
constexpr wchar_t kPolishLanguage[] = L"Language=415";
}  // namespace

Speech::Speech() {
    if (FAILED(voice_.CoCreateInstance(CLSID_SpVoice))) {
        return;  // COM/SAPI unavailable -> Say() becomes a no-op
    }
    CComPtr<ISpObjectToken> voiceToken;
    if (SUCCEEDED(SpFindBestToken(SPCAT_VOICES, kPolishLanguage, L"", &voiceToken)) &&
        voiceToken != nullptr) {
        voice_->SetVoice(voiceToken);
    }
}

void Speech::Say(const CString& text) {
    if (voice_ != nullptr) {
        voice_->Speak(text, SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    }
}
