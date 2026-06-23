#include "framework.h"

#include "DarkVisualManager.h"
#include "RecordMovementDialog.h"
#include "TextUtil.h"
#include "warehouse/view_logic.hpp"

BEGIN_MESSAGE_MAP(CRecordMovementDialog, CDialogEx)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

namespace {
constexpr int kMinQty = 1;
constexpr int kMaxQty = 1000000;

// Fill a combo and pre-select the option whose id matches selectedId (the grid's current
// row); selectedIndexForId (core, unit-tested) falls back to the first item when none matches.
void fill(CComboBox& combo, const std::vector<ComboOption>& options, int selectedId) {
    std::vector<int> ids;
    ids.reserve(options.size());
    for (const ComboOption& option : options) {
        const int index = combo.AddString(option.label);
        combo.SetItemData(index, static_cast<DWORD_PTR>(option.id));
        ids.push_back(option.id);
    }
    if (!options.empty()) {
        combo.SetCurSel(warehouse::selectedIndexForId(ids, selectedId));
    }
}
}  // namespace

CRecordMovementDialog::CRecordMovementDialog(warehouse::MovementType type,
                                             std::vector<ComboOption> products,
                                             std::vector<ComboOption> warehouses,
                                             CWnd* parent)
    : CDialogEx(IDD_RECORD_MOVEMENT, parent),
      type_(type),
      products_(std::move(products)),
      warehouses_(std::move(warehouses)) {}

void CRecordMovementDialog::DoDataExchange(CDataExchange* dx) {
    CDialogEx::DoDataExchange(dx);
    DDX_Control(dx, IDC_PRODUCT, productCombo_);
    DDX_Control(dx, IDC_WAREHOUSE, warehouseCombo_);
    // Quantity is validated by hand in OnOK so the range error is Polish and titled,
    // instead of the framework's English DDV_MinMaxInt message box.
}

BOOL CRecordMovementDialog::OnInitDialog() {
    CDialogEx::OnInitDialog();
    // Match the app's active theme: under the custom dark visual manager, paint the dialog
    // background dark (CDialogEx fills it + tints the static labels via OnCtlColor below).
    dark_ = CMFCVisualManager::GetInstance()->IsKindOf(RUNTIME_CLASS(CDarkVisualManager)) != FALSE;
    if (dark_) {
        SetBackgroundColor(ThemeColorsFor(true).background);
    }
    SetWindowText(type_ == warehouse::MovementType::In ? _T("Przyjęcie (IN)")
                                                       : _T("Wydanie (OUT)"));
    fill(productCombo_, products_, initialProductId_);
    fill(warehouseCombo_, warehouses_, initialWarehouseId_);
    SetDlgItemInt(IDC_QTY, qty_, FALSE);  // default quantity (unsigned: the field is ES_NUMBER)
    // Product/warehouse come pre-set from the grid, so go straight to the quantity field
    // (with its text selected, ready to overtype).
    GotoDlgCtrl(GetDlgItem(IDC_QTY));
    SendDlgItemMessage(IDC_QTY, EM_SETSEL, 0, -1);
    return FALSE;  // FALSE: focus was set explicitly above
}

void CRecordMovementDialog::OnOK() {
    const int product = productCombo_.GetCurSel();
    const int warehouse = warehouseCombo_.GetCurSel();
    if (product == CB_ERR || warehouse == CB_ERR) {
        MessageBox(_T("Wybierz produkt i magazyn."), kAppTitle, MB_ICONWARNING);
        return;
    }

    BOOL parsed = FALSE;
    const int qty = static_cast<int>(GetDlgItemInt(IDC_QTY, &parsed, FALSE));  // FALSE: unsigned
    if (!parsed || qty < kMinQty || qty > kMaxQty) {
        CString message;
        message.Format(_T("Podaj ilość z zakresu od %d do %d."), kMinQty, kMaxQty);
        MessageBox(message, kAppTitle, MB_ICONWARNING);
        GotoDlgCtrl(GetDlgItem(IDC_QTY));
        return;
    }

    qty_ = qty;
    productId_ = static_cast<int>(productCombo_.GetItemData(product));
    warehouseId_ = static_cast<int>(warehouseCombo_.GetItemData(warehouse));
    CDialogEx::OnOK();
}

// In dark mode, draw the static labels' text light over the dark dialog background
// (CDialogEx already returns the matching background brush for the controls).
HBRUSH CRecordMovementDialog::OnCtlColor(CDC* dc, CWnd* wnd, UINT type) {
    HBRUSH brush = CDialogEx::OnCtlColor(dc, wnd, type);
    if (dark_ && type == CTLCOLOR_STATIC) {
        dc->SetTextColor(ThemeColorsFor(true).text);
        dc->SetBkMode(TRANSPARENT);
    }
    return brush;
}
