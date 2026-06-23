#pragma once

#include "framework.h"

// The MFC application object. One global instance (theApp) is the entry point.
// CWinAppEx enables the MFC Feature Pack (visual managers, docking, ribbon).
class CWarehouseApp : public CWinAppEx {
public:
    BOOL InitInstance() override;

    // Stop the Feature Pack from writing the current docking/window layout on exit
    // (used by "Reset layout", which clears the saved state and restarts). m_bSaveState
    // is protected on CWinAppEx, so it's exposed here.
    void SkipStateSaveOnExit() { m_bSaveState = FALSE; }
};

extern CWarehouseApp theApp;
