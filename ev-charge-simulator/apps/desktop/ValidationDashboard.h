#pragma once

#include <QDialog>

#include <vector>

#include "ev/validation/ValidationSuite.h"

class QLabel;
class QProgressBar;
class QTableWidget;

/// Read-only engineering dashboard. It invokes the real domain engine via the quick-check suite.
/// Does not change the active scenario and does not substitute the GoogleTest / CTest CI suite.
class ValidationDashboard : public QDialog {
public:
    explicit ValidationDashboard(QWidget* parent = nullptr);

private:
    void runChecks();
    void exportCsv();

    QLabel* passedValue_ = nullptr;
    QLabel* failedValue_ = nullptr;
    QLabel* totalValue_ = nullptr;
    QLabel* runTime_ = nullptr;
    QProgressBar* progress_ = nullptr;
    QTableWidget* table_ = nullptr;
    std::vector<ev::validation::CheckResult> checks_;
};
