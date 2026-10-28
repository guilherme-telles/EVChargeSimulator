#pragma once

#include <QMainWindow>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ev/app/SimulationService.h"
#include "ev/domain/Ports.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QTableWidget;
class QToolButton;
class QWidget;
class QFormLayout;
class QCheckBox;

/// View only: reads inputs, calls SimulationService and binds ResultView strings. No formulas here (spec 5.1).
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(ev::app::SimulationService& service, const ev::domain::IVehicleCatalog* catalog,
               QWidget* parent = nullptr);

private slots:
    void calculate();
    void onVehicleSelected(int index);
    void onChargerTypeChanged(int index);
    void addToComparison();
    void clearComparison();
    void openScenario();
    void saveScenario();
    void copySummary();
    void addCurveRow();
    void removeCurveRow();

private:
    QWidget* buildInputPanel();
    QWidget* buildResultPanel();
    QWidget* buildComparisonPanel();
    void buildMenu();
    QDoubleSpinBox* addField(QFormLayout* form, const QString& label, const char* fieldId, double min, double max,
                             int decimals, const QString& suffix, const QString& specialText = {});
    QLabel* addErrorLabel(const char* fieldId);

    [[nodiscard]] std::optional<ev::domain::ChargeRequest> readForm();
    void writeForm(const ev::domain::ChargeRequest& request);
    void clearErrors();
    void clearResult();
    void showErrors(const std::vector<ev::domain::ValidationError>& errors);
    void showResult(const ev::domain::ChargeRequest& request, const ev::domain::ChargeResult& result);
    void refreshComparison();

    ev::app::SimulationService& service_;
    std::vector<ev::domain::Vehicle> vehicles_;

    QComboBox* vehicleCombo_ = nullptr;
    QComboBox* chargerType_ = nullptr;
    QDoubleSpinBox* capacity_ = nullptr;
    QDoubleSpinBox* vehiclePower_ = nullptr;
    QDoubleSpinBox* consumption_ = nullptr;
    QDoubleSpinBox* ratedRange_ = nullptr;
    QDoubleSpinBox* chargerPower_ = nullptr;
    QDoubleSpinBox* installationLimit_ = nullptr;
    QDoubleSpinBox* currentSoc_ = nullptr;
    QDoubleSpinBox* targetSoc_ = nullptr;
    QDoubleSpinBox* efficiency_ = nullptr;
    QDoubleSpinBox* energyRate_ = nullptr;
    QDoubleSpinBox* fixedFee_ = nullptr;
    QDoubleSpinBox* minuteRate_ = nullptr;
    QCheckBox* useCurve_ = nullptr;
    QTableWidget* curveTable_ = nullptr;

    QLabel* errorSummary_ = nullptr;
    QLabel* timeCard_ = nullptr;
    QLabel* costCard_ = nullptr;
    QLabel* rangeCard_ = nullptr;
    QToolButton* detailsToggle_ = nullptr;
    QWidget* details_ = nullptr;
    QLabel* detailsText_ = nullptr;
    QLabel* warnings_ = nullptr;
    QTableWidget* comparisonTable_ = nullptr;

    std::map<std::string, QWidget*> fieldWidgets_;
    std::map<std::string, QLabel*> fieldErrors_;
    std::optional<std::pair<ev::domain::ChargeRequest, ev::domain::ChargeResult>> last_;
};
