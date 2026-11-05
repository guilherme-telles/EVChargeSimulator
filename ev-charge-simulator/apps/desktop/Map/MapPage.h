
#pragma once

#include <QWidget>
#include <QShowEvent>

class MapBridge;
class QWebChannel;
class QWebEngineView;
class QLabel;

class MapPage final : public QWidget
{
    Q_OBJECT

public:
    explicit MapPage(QWidget* parent = nullptr);

    void setVehicleData(
        double capacityKwh,
        double currentSocPercent,
        double consumptionKwhPer100Km,
        double ratedRangeKm
    );

    void setTripSummary(
        double rangeKm,
        double routeKm,
        double energyKwh,
        double arrivalSocPercent
    );

protected:
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void setupWebChannel();
    void updateVehicleIndicators();
    void updateTripIndicators(double distanceKm, double durationMinutes);

    double routeDistanceKm_ = 0.0;
    double routeDurationMinutes_ = 0.0;
    bool hasRoute_ = false;
    
    // Dados atuais do veículo e da bateria.
    double capacityKwh_ = 0.0;
    double currentSocPercent_ = 0.0;
    double consumptionKwhPer100Km_ = 0.0;
    double ratedRangeKm_ = 0.0;

    QWebEngineView* webView_ = nullptr;

    MapBridge* bridge_ = nullptr;
    QWebChannel* webChannel_ = nullptr;

    QLabel* rangeValue_ = nullptr;
    QLabel* routeValue_ = nullptr;
    QLabel* energyValue_ = nullptr;
    QLabel* arrivalSocValue_ = nullptr;
    QLabel* statusLabel_ = nullptr;


};
