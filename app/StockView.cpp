#include "framework.h"
#include "resource.h"

#include <set>
#include <string>
#include <thread>
#include <vector>

#include "MicCapture.h"
#include "RecordMovementDialog.h"
#include "StockView.h"
#include "Stt.h"
#include "TextUtil.h"
#include "WarehouseDoc.h"
#include "warehouse/voice_command_parser.hpp"

IMPLEMENT_DYNCREATE(CStockView, CListView)

BEGIN_MESSAGE_MAP(CStockView, CListView)
    ON_COMMAND(ID_STOCK_REFRESH, &CStockView::OnStockRefresh)
    ON_COMMAND(ID_STOCK_RECORD_IN, &CStockView::OnRecordIn)
    ON_COMMAND(ID_STOCK_RECORD_OUT, &CStockView::OnRecordOut)
    ON_COMMAND(ID_EDIT_UNDO, &CStockView::OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, &CStockView::OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CStockView::OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CStockView::OnUpdateEditRedo)
    ON_COMMAND(ID_STOCK_FILTER_LOW, &CStockView::OnFilterLow)
    ON_UPDATE_COMMAND_UI(ID_STOCK_FILTER_LOW, &CStockView::OnUpdateFilterLow)
    ON_COMMAND(ID_VOICE_LISTEN, &CStockView::OnVoiceListen)
    ON_MESSAGE(WM_STT_RESULT, &CStockView::OnSttResult)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CStockView::OnCustomDraw)
END_MESSAGE_MAP()

namespace {
constexpr int kColWarehouse = 0;
constexpr int kColSku = 1;
constexpr int kColProduct = 2;
constexpr int kColOnHand = 3;
}  // namespace

CStockView::CStockView() {}
CStockView::~CStockView() = default;

CWarehouseDoc* CStockView::GetDocument() const {
    return static_cast<CWarehouseDoc*>(m_pDocument);
}

BOOL CStockView::PreCreateWindow(CREATESTRUCT& cs) {
    cs.style |= LVS_REPORT;
    return CListView::PreCreateWindow(cs);
}

void CStockView::OnInitialUpdate() {
    CListCtrl& list = GetListCtrl();
    if (list.GetHeaderCtrl()->GetItemCount() == 0) {
        list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        list.InsertColumn(kColWarehouse, _T("Magazyn"), LVCFMT_LEFT, 90);
        list.InsertColumn(kColSku, _T("SKU"), LVCFMT_LEFT, 60);
        list.InsertColumn(kColProduct, _T("Produkt"), LVCFMT_LEFT, 220);
        list.InsertColumn(kColOnHand, _T("Stan"), LVCFMT_RIGHT, 70);
    }
    CListView::OnInitialUpdate();  // triggers OnUpdate to fill the rows
}

void CStockView::OnUpdate(CView* /*sender*/, LPARAM /*hint*/, CObject* /*hintObject*/) {
    const CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr) {
        return;
    }
    CListCtrl& list = GetListCtrl();
    list.DeleteAllItems();

    int item = 0;
    for (const warehouse::StockRow& stock : doc->Stock()) {
        if (showLowOnly_ && !stock.isLow) {
            continue;
        }
        list.InsertItem(item, FromUtf8(stock.warehouseCode));
        list.SetItemText(item, kColSku, FromUtf8(stock.sku));
        list.SetItemText(item, kColProduct, FromUtf8(stock.productName));
        CString onHand;
        onHand.Format(_T("%d"), stock.onHand);
        list.SetItemText(item, kColOnHand, onHand);
        list.SetItemData(item, stock.isLow ? 1 : 0);  // drives the red custom draw
        ++item;
    }
}

void CStockView::OnStockRefresh() {
    if (CWarehouseDoc* doc = GetDocument()) {
        doc->Refresh();
    }
}

void CStockView::OnRecordIn() { RecordMovement(warehouse::MovementType::In); }
void CStockView::OnRecordOut() { RecordMovement(warehouse::MovementType::Out); }

void CStockView::RecordMovement(warehouse::MovementType type) {
    CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr) {
        return;
    }

    // Distinct products and warehouses for the dialog combos, taken from the data.
    std::vector<ComboOption> products;
    std::vector<ComboOption> warehouses;
    std::set<int> seenProducts;
    std::set<int> seenWarehouses;
    for (const warehouse::StockRow& row : doc->Stock()) {
        if (seenProducts.insert(row.productId).second) {
            products.push_back({row.productId, FromUtf8(row.sku + " " + row.productName)});
        }
        if (seenWarehouses.insert(row.warehouseId).second) {
            warehouses.push_back({row.warehouseId, FromUtf8(row.warehouseCode + " " + row.warehouseName)});
        }
    }

    CRecordMovementDialog dialog(type, std::move(products), std::move(warehouses), this);
    if (dialog.DoModal() == IDOK) {
        doc->ExecuteMovement(
            warehouse::Movement{dialog.productId(), dialog.warehouseId(), dialog.qty(), type});
    }
}

void CStockView::OnEditUndo() {
    if (CWarehouseDoc* doc = GetDocument()) {
        doc->Undo();
    }
}

void CStockView::OnEditRedo() {
    if (CWarehouseDoc* doc = GetDocument()) {
        doc->Redo();
    }
}

void CStockView::OnUpdateEditUndo(CCmdUI* cmdUI) {
    const CWarehouseDoc* doc = GetDocument();
    cmdUI->Enable(doc != nullptr && doc->CanUndo());
}

void CStockView::OnUpdateEditRedo(CCmdUI* cmdUI) {
    const CWarehouseDoc* doc = GetDocument();
    cmdUI->Enable(doc != nullptr && doc->CanRedo());
}

void CStockView::OnFilterLow() { ShowLowOnly(!showLowOnly_); }

void CStockView::ShowLowOnly(bool on) {
    showLowOnly_ = on;
    OnUpdate(nullptr, 0, nullptr);  // re-populate with the filter applied
}

void CStockView::OnUpdateFilterLow(CCmdUI* cmdUI) {
    cmdUI->SetCheck(showLowOnly_ ? 1 : 0);
}

// --- Voice command path (offline Polish STT via whisper.cpp) ---------------

Stt* CStockView::EnsureStt() {
    if (!sttLoadTried_) {
        sttLoadTried_ = true;
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring path(exePath);
        const std::size_t slash = path.find_last_of(L"\\/");
        const std::wstring dir = (slash == std::wstring::npos) ? L"" : path.substr(0, slash + 1);
        auto stt = std::make_unique<Stt>(dir + L"ggml-base.bin");
        if (stt->ok()) {
            stt_ = std::move(stt);
        }
    }
    return stt_.get();
}

void CStockView::OnVoiceListen() {
    if (listening_.load()) {
        return;  // a capture is already in flight
    }
    if (EnsureStt() == nullptr) {
        AfxMessageBox(_T("Model głosowy (ggml-base.bin) niedostępny obok aplikacji."),
                      MB_ICONWARNING);
        return;
    }

    listening_.store(true);
    MessageBeep(MB_OK);  // audible cue: start speaking now

    // Capture + recognise off the UI thread, then post the text back to ourselves.
    const HWND target = GetSafeHwnd();
    Stt* stt = stt_.get();
    std::thread([target, stt]() {
        const std::vector<float> audio = mic::Record(4);
        std::string text = stt->Transcribe(audio);
        ::PostMessage(target, WM_STT_RESULT, 0,
                      reinterpret_cast<LPARAM>(new std::string(std::move(text))));
    }).detach();
}

LRESULT CStockView::OnSttResult(WPARAM /*wParam*/, LPARAM lParam) {
    std::unique_ptr<std::string> text(reinterpret_cast<std::string*>(lParam));
    listening_.store(false);
    if (text) {
        DispatchVoice(*text);
    }
    return 0;
}

void CStockView::DispatchVoice(const std::string& utf8Text) {
    CWarehouseDoc* doc = GetDocument();
    if (doc == nullptr) {
        return;
    }

    const warehouse::ParsedCommand command = warehouse::parseVoiceCommand(utf8Text);
    switch (command.kind) {
        case warehouse::CommandKind::Refresh:
            doc->Refresh();
            return;
        case warehouse::CommandKind::Undo:
            doc->Undo();
            return;
        case warehouse::CommandKind::Redo:
            doc->Redo();
            return;
        case warehouse::CommandKind::ShowLowStock:
            ShowLowOnly(true);
            return;
        case warehouse::CommandKind::RecordMovement:
            // The parser stays DB-free; resolve the spoken sku to ids from the snapshot.
            for (const warehouse::StockRow& row : doc->Stock()) {
                if (row.sku == command.sku) {
                    doc->ExecuteMovement(warehouse::Movement{row.productId, row.warehouseId,
                                                             command.qty, command.type});
                    return;
                }
            }
            AfxMessageBox(FromUtf8("Nie znam artykułu: " + command.sku), MB_ICONWARNING);
            return;
        case warehouse::CommandKind::Unknown:
        default:
            AfxMessageBox(FromUtf8("Nie rozpoznano polecenia: " + utf8Text), MB_ICONINFORMATION);
            return;
    }
}

// Paint low-stock rows in red (OnHand <= ReorderLevel, per the IsLow flag).
void CStockView::OnCustomDraw(NMHDR* notify, LRESULT* result) {
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(notify);
    switch (draw->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            *result = CDRF_NOTIFYITEMDRAW;
            return;
        case CDDS_ITEMPREPAINT: {
            const int row = static_cast<int>(draw->nmcd.dwItemSpec);
            if (GetListCtrl().GetItemData(row) != 0) {  // 1 == low stock
                draw->clrText = RGB(192, 0, 0);
            }
            *result = CDRF_DODEFAULT;
            return;
        }
        default:
            *result = CDRF_DODEFAULT;
            return;
    }
}
