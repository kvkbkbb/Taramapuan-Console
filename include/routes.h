#ifndef ROUTES_H
#define ROUTES_H

#include "ds/array.h"
#include "ds/graph.h"
#include "models.h"

#include <string>

namespace routes {

void initialize();
const Array<Shuttle>& shuttles();
const Shuttle* findShuttle(int shuttleNumber);
const Graph& network();
const Array<std::string>& stopNames();
void printPickupSequence();
void printShuttlePath(int shuttleNumber);
void printBfsRoute(int shuttleNumber);

// ==========================================
// Non-breaking Dynamic Route Extensions
// ==========================================
Array<std::string> getStopList(int shuttleNumber, bool isIncoming);
Array<int> computeDynamicPath(int shuttleNumber, bool isIncoming, int chosenStopIndex);
bool dynamicRouteDetails(int shuttleNumber, bool isIncoming, const Array<int>& path,
                         std::string& routeStops, double& totalDistance);
bool combinedRouteDetails(int shuttleNumber, bool isIncoming,
                          const Array<int>& selectedStops,
                          std::string& routeStops, double& totalDistance);
double estimatedDurationMinutes(double distanceKm);
void printDynamicRoute(int shuttleNumber, bool isIncoming, const Array<int>& path);
bool scheduledRouteDetails(int shuttleNumber, int scheduleNumber,
                           std::string& routeStops, double& totalDistance);

}

#endif