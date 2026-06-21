#pragma once

#include <atlbase.h>
#include <sapi.h>

#include "framework.h"

// SAPI text-to-speech, preferring a Polish voice (Microsoft Paulina). Construction
// and Say() are no-ops if speech is unavailable, so callers never need to guard.
class Speech {
public:
    Speech();
    void Say(const CString& text);

private:
    CComPtr<ISpVoice> voice_;
};
