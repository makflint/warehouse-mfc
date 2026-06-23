#pragma once

#include <vector>

#include <afxdialogex.h>

#include "framework.h"
#include "resource.h"
#include "warehouse/movement.hpp"

// One choice in a combo box: the database id plus the text shown to the user.
struct ComboOption {
    int id;
    CString label;
};

// Modal dialog to capture a stock movement: product, warehouse and quantity.
// The direction (IN/OUT) is fixed by the command that opened the dialog.
// CDialogEx (+ visual-manager style) so it adopts the app's active theme, incl. dark.
class CRecordMovementDialog : public CDialogEx {
public:
    CRecordMovementDialog(warehouse::MovementType type,
                          std::vector<ComboOption> products,
                          std::vector<ComboOption> warehouses,
                          CWnd* parent = nullptr);

    // Pre-select the product/warehouse to match the grid's current row (call before DoModal).
    void Preselect(int productId, int warehouseId) {
        initialProductId_ = productId;
        initialWarehouseId_ = warehouseId;
    }

    int productId() const { return productId_; }
    int warehouseId() const { return warehouseId_; }
    int qty() const { return qty_; }

protected:
    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* dx) override;
    void OnOK() override;
    afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT type);
    DECLARE_MESSAGE_MAP()

private:
    warehouse::MovementType type_;
    bool dark_ = false;  // active visual manager is the custom dark one
    std::vector<ComboOption> products_;
    std::vector<ComboOption> warehouses_;
    CComboBox productCombo_;
    CComboBox warehouseCombo_;
    int qty_ = 1;
    int productId_ = 0;
    int warehouseId_ = 0;
    int initialProductId_ = 0;    // grid row to pre-select in the combos
    int initialWarehouseId_ = 0;
};
