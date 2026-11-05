#include "MapBridge.h"

MapBridge::MapBridge(QObject* parent): QObject(parent){
}

void MapBridge::mapReady(){
    emit pageReady();
}

void MapBridge::clearRoute(){
    emit routeCleared();
}

void MapBridge::coordinatesSelected(double latitude, double longitude){
    emit coordinateSelected(latitude, longitude);
}

void MapBridge::routeRequested(double fromLat, double fromLon, double toLat, double toLon){
    emit routeRequestedFromMap(fromLat, fromLon, toLat, toLon);
}

void MapBridge::routeCalculated(double distanceKm, double durationMinutes){
    emit routeCalculatedFromMap(distanceKm, durationMinutes);
}