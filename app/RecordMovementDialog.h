#pragma once

#include <vector>

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
class CRecordMovementDialog : public CDialog {
public:
    CRecordMovementDialog(warehouse::MovementType type,
                          std::vector<ComboOption> products,
                          std::vector<ComboOption> warehouses,
                          CWnd* parent = nullptr);

    int productId() const { return productId_; }
    int warehouseId() const { return warehouseId_; }
    int qty() const { return qty_; }

protected:
    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* dx) override;
    void OnOK() override;

private:
    warehouse::MovementType type_;
    std::vector<ComboOption> products_;
    std::vector<ComboOption> warehouses_;
    CComboBox productCombo_;
    CComboBox warehouseCombo_;
    int qty_ = 1;
    int productId_ = 0;
    int warehouseId_ = 0;
};
