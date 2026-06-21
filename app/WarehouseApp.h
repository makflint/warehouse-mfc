#pragma once

#include "framework.h"

// The MFC application object. One global instance (theApp) is the entry point.
class CWarehouseApp : public CWinApp {
public:
    BOOL InitInstance() override;
};

extern CWarehouseApp theApp;
