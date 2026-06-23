#include "framework.h"

#include "RecordMovementDialog.h"
#include "TextUtil.h"

namespace {
constexpr int kMinQty = 1;
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
    // Quantity is validated by hand in OnOK so the range error is Polish and titled,
    // instead of the framework's English DDV_MinMaxInt message box.
}

BOOL CRecordMovementDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText(type_ == warehouse::MovementType::In ? _T("Przyjęcie (IN)")
                                                       : _T("Wydanie (OUT)"));
    fill(productCombo_, products_);
    fill(warehouseCombo_, warehouses_);
    SetDlgItemInt(IDC_QTY, qty_, FALSE);  // default quantity (unsigned: the field is ES_NUMBER)
    return TRUE;
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
