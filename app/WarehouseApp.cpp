#include "framework.h"
#include "resource.h"

#include <string>

#include "WarehouseApp.h"
#include "MainFrame.h"
#include "StockView.h"
#include "WarehouseDoc.h"
#include "warehouse/connection_profiles.hpp"
#include "warehouse/database_initializer.hpp"

CWarehouseApp theApp;

namespace {
// Directory of the running executable, as a UTF-8 path.
std::string exeDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring path(buffer);
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring dir = (slash == std::wstring::npos) ? L"" : path.substr(0, slash);
    const int length = ::WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(length > 0 ? length - 1 : 0, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), -1, result.data(), length, nullptr, nullptr);
    return result;
}
}  // namespace

BOOL CWarehouseApp::InitInstance() {
    CWinApp::InitInstance();
    AfxOleInit();  // COM for SAPI text-to-speech

    // First run of the packaged demo: create + seed the LocalDB database if missing.
    const std::string dir = exeDirectory();
    warehouse::ensureDatabase(warehouse::demoMasterConnectionString(),
                              dir + "\\db\\01_schema.sql", dir + "\\db\\02_seed.sql");

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
