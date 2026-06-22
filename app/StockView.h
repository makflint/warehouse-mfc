#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "framework.h"
#include "warehouse/movement.hpp"

class CWarehouseDoc;
class Stt;

// The stock grid: a report-style list showing vCurrentStock.
class CStockView : public CListView {
    DECLARE_DYNCREATE(CStockView)

public:
    CStockView();
    ~CStockView() override;  // out-of-line so unique_ptr<Stt> sees a complete type

    CWarehouseDoc* GetDocument() const;

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    void OnInitialUpdate() override;
    void OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) override;

protected:
    afx_msg void OnStockRefresh();
    afx_msg void OnRecordIn();
    afx_msg void OnRecordOut();
    afx_msg void OnEditUndo();
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditUndo(CCmdUI* cmdUI);
    afx_msg void OnUpdateEditRedo(CCmdUI* cmdUI);
    afx_msg void OnFilterLow();
    afx_msg void OnUpdateFilterLow(CCmdUI* cmdUI);
    afx_msg void OnVoiceListen();
    afx_msg LRESULT OnSttResult(WPARAM wParam, LPARAM lParam);
    afx_msg void OnCustomDraw(NMHDR* notify, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    void RecordMovement(warehouse::MovementType type);

    // Voice: lazily load the model, then dispatch one recognised phrase through the
    // (unit-tested) core parser to the same document commands the menus use.
    Stt* EnsureStt();
    void DispatchVoice(const std::string& utf8Text);
    void ShowLowOnly(bool on);

    bool showLowOnly_ = false;
    bool sttLoadTried_ = false;
    std::unique_ptr<Stt> stt_;
    std::atomic<bool> listening_{false};
    CString savedTitle_;  // frame title restored after the "Słucham…" cue
};
