#include "framework.h"

#include "RecordMovementDialog.h"

namespace {
constexpr int kMaxQty = 1000000;

void fill(CComboBox& combo, const std::vector<ComboOption>& options) {
    for (const ComboOption& option : options) {
        const int index = combo.AddString(option.label);
        combo.SetItemData(index, static_cast<DWORD_PTR>(option.id));
    }
    if (!options.empty()) {
        combo.SetCurSel(0);
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
    DDX_Text(dx, IDC_QTY, qty_);
    DDV_MinMaxInt(dx, qty_, 1, kMaxQty);
}

BOOL CRecordMovementDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText(type_ == warehouse::MovementType::In ? _T("Przyjecie (IN)")
                                                       : _T("Wydanie (OUT)"));
    fill(productCombo_, products_);
    fill(warehouseCombo_, warehouses_);
    return TRUE;
}

void CRecordMovementDialog::OnOK() {
    if (!UpdateData(TRUE)) {
        return;
    }
    const int product = productCombo_.GetCurSel();
    const int warehouse = warehouseCombo_.GetCurSel();
    if (product == CB_ERR || warehouse == CB_ERR) {
        AfxMessageBox(_T("Wybierz produkt i magazyn."), MB_ICONWARNING);
        return;
    }
    productId_ = static_cast<int>(productCombo_.GetItemData(product));
    warehouseId_ = static_cast<int>(warehouseCombo_.GetItemData(warehouse));
    CDialog::OnOK();
}
