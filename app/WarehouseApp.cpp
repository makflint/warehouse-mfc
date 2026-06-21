#include "framework.h"
#include "resource.h"

#include "WarehouseApp.h"
#include "MainFrame.h"
#include "StockView.h"
#include "WarehouseDoc.h"

CWarehouseApp theApp;

BOOL CWarehouseApp::InitInstance() {
    CWinApp::InitInstance();
    AfxOleInit();  // COM for SAPI text-to-speech

    // SDI doc/view: one document, the stock grid as its view.
    auto* docTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CWarehouseDoc),
        RUNTIME_CLASS(CMainFrame),
        RUNTIME_CLASS(CStockView));
    AddDocTemplate(docTemplate);

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    if (!ProcessShellCommand(cmdInfo)) {
        return FALSE;
    }

    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();
    return TRUE;
}
