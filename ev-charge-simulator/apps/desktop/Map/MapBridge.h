#pragma once

#include <QObject>

class MapBridge final : public QObject {
    Q_OBJECT

public:
    explicit MapBridge(QObject* parent = nullptr);

public slots:
    void mapReady();
    void coordinatesSelected(double latitude, double longitude);
    void routeRequested(double fromLat, double fromLon, double toLat, double toLon);
    void routeCalculated(double distanceKm, double durationMinutes);
    void clearRoute();

signals:
    void pageReady();
    void coordinateSelected(double latitude, double longitude);
    void routeRequestedFromMap(double fromLat, double fromLon, double toLat, double toLon);
    void routeCalculatedFromMap(double distanceKm, double durationMinutes);
    void routeCleared();
    
};