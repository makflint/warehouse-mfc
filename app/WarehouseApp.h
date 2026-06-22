#pragma once

#include "framework.h"

// The MFC application object. One global instance (theApp) is the entry point.
// CWinAppEx enables the MFC Feature Pack (visual managers, docking, ribbon).
class CWarehouseApp : public CWinAppEx {
public:
    BOOL InitInstance() override;
};

extern CWarehouseApp theApp;
