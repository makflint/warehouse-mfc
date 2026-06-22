#pragma once

#include "framework.h"

// CFrameWndEx is the Feature Pack frame — it hosts the ribbon, dockable panes and
// honours the active visual manager (theming).
class CMainFrame : public CFrameWndEx {
    DECLARE_DYNCREATE(CMainFrame)

public:
    CMainFrame();

protected:
    DECLARE_MESSAGE_MAP()
};
