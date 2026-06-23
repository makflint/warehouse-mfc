#include "framework.h"

#include "RecordMovementDialog.h"
#include "TextUtil.h"

namespace {
constexpr int kMinQty = 1;
constexpr int kMaxQty = 1000000;

// Fill a combo and pre-select the option whose id matches selectedId (the grid's current
// row); falls back to the first item when there's no match.
void fill(CComboBox& combo, const std::vector<ComboOption>& options, int selectedId) {
    int selected = 0;
    for (std::size_t i = 0; i < options.size(); ++i) {
        const int index = combo.AddString(options[i].label);
        combo.SetItemData(index, static_cast<DWORD_PTR>(options[i].id));
        if (options[i].id == selectedId) {
            selected = index;
        }
    }
    if (!options.empty()) {
        combo.SetCurSel(selected);
    }
}
}  // namespace

CRecordMovementDialog::CRecordMovementDialog(warehouse::MovementType type,
                                             std::vector<ComboOption> products,
                                             std::vector<ComboOption> warehouses,
                                             CWnd* parent)
    : CDialog(IDD_RECORD_MOVEMENT, parent),
      type_(type),
      products_(std::move(products)),
      warehouses_(std::move(warehouses)) {}

void CRecordMovementDialog::DoDataExchange(CDataExchange* dx) {
    CDialog::DoDataExchange(dx);
    DDX_Control(dx, IDC_PRODUCT, productCombo_);
    DDX_Control(dx, IDC_WAREHOUSE, warehouseCombo_);
    // Quantity is validated by hand in OnOK so the range error is Polish and titled,
    // instead of the framework's English DDV_MinMaxInt message box.
}

BOOL CRecordMovementDialog::OnInitDialog() {
    CDialog::OnInitDialog();
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
    CDialog::OnOK();
}
