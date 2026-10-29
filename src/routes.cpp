#include "routes.h"

#include <iomanip>
#include <iostream>

namespace {

Array<Shuttle> catalog;
Array<std::string> names; // Global default stop names
bool ready = false;

const double spPickupDistances[6][6] = {
    {0.0, 1.8, 0.0, 0.0, 0.0, 0.0},
    {1.8, 0.0, 1.3, 0.0, 0.0, 0.0},
    {0.0, 1.3, 0.0, 1.7, 0.0, 0.0},
    {0.0, 0.0, 1.7, 0.0, 2.2, 0.0},
    {0.0, 0.0, 0.0, 2.2, 0.0, 13.5},
    {0.0, 0.0, 0.0, 0.0, 13.5, 0.0}
};

const double spDropoffDistances[10][10] = {
    {0.0, 11.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {11.5, 0.0, 2.8, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 2.8, 0.0, 2.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 2.0, 2.2, 0.0, 1.7, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 1.7, 0.0, 1.3, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 1.3, 0.0, 1.5, 2.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 1.5, 0.0, 1.8, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 2.0, 1.8, 0.0, 2.5, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.5, 0.0, 1.2},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.2, 0.0}
};

const double staRosaDistances[7][7] = {
    {0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {1.0, 0.0, 8.0, 7.0, 7.0, 1.0, 0.0},
    {0.0, 8.0, 0.0, 7.0, 1.0, 0.0, 0.0},
    {0.0, 7.0, 7.0, 0.0, 6.0, 6.0, 0.0},
    {0.0, 7.0, 1.0, 6.0, 0.0, 0.0, 0.5},
    {0.0, 1.0, 0.0, 6.0, 0.0, 0.0, 1.0},
    {0.0, 0.0, 0.0, 0.0, 0.5, 1.0, 0.0}
};

const double calambaDistances[4][4] = {
    {0.0, 2.0, 0.0, 0.0},
    {2.0, 0.0, 3.0, 0.0},
    {0.0, 3.0, 0.0, 4.0},
    {0.0, 0.0, 4.0, 0.0}
};

// 1. San Pedro - Incoming (Pickup - 6 nodes: 0 to 5)
// Astrodome (0), Pacita 1 (1), Chrysanthemum (2), Rosario (3), Southwoods (4), MCL (5)
Graph spPickupGraph(6);
Array<std::string> spPickupStops;

// 2. San Pedro - Outgoing (Drop-off - 10 nodes: 0 to 9)
// MCL (0), Halang (1), SouthwoodsMall (2), Rosario (3), Chrysanthemum (4),
// Pacita 1 (5), Macaria (6), Labasan (7), Galleria (8), SanPedro(United) (9)
Graph spDropoffGraph(10);
Array<std::string> spDropoffStops;

// 3. Sta. Rosa (Paseo) - 7 nodes: 0 to 6
// MMCL (0), Pramana (1), Zadia (2), Solen (3), Paseo (4), Belair (5), Don Jose (6)
Graph staRosaGraph(7);
Array<std::string> staRosaStops;

// 4. Calamba - 4 nodes: 0 to 3
// MMCL (0), Batino (1), Real (2), SM (3)
Graph calambaGraph(4);
Array<std::string> calambaStops;

double edgeDistance(int shuttleNumber, bool isIncoming, int from, int to) {
    if (shuttleNumber == 1) {
        if (isIncoming) {
            return from >= 0 && from < 6 && to >= 0 && to < 6
                ? spPickupDistances[from][to] : 0.0;
        }
        return from >= 0 && from < 10 && to >= 0 && to < 10
            ? spDropoffDistances[from][to] : 0.0;
    }
    if (shuttleNumber == 2) {
        return from >= 0 && from < 7 && to >= 0 && to < 7
            ? staRosaDistances[from][to] : 0.0;
    }
    if (shuttleNumber == 3) {
        return from >= 0 && from < 4 && to >= 0 && to < 4
            ? calambaDistances[from][to] : 0.0;
    }
    return 0.0;
}

bool buildScheduledPath(int shuttleNumber, int scheduleNumber,
                        bool& isIncoming, Array<int>& path) {
    const Shuttle* shuttle = routes::findShuttle(shuttleNumber);
    if (shuttle == nullptr || scheduleNumber < 1
        || scheduleNumber > shuttle->schedules.size()) {
        return false;
    }

    isIncoming = shuttle->schedules[scheduleNumber - 1].direction.find("Incoming") == 0;
    if (shuttleNumber == 1) {
        path = isIncoming ? spPickupGraph.bfsPath(0, 5)
                          : spDropoffGraph.bfsPath(0, 9);
    } else if (shuttleNumber == 2) {
        path = isIncoming ? staRosaGraph.bfsPath(4, 0)
                          : staRosaGraph.bfsPath(0, 4);
    } else {
        path = isIncoming ? calambaGraph.bfsPath(3, 0)
                          : calambaGraph.bfsPath(0, 3);
    }
    return !path.empty();
}

bool formatPathDetails(int shuttleNumber, bool isIncoming, const Array<int>& path,
                       std::string& routeStops, double& totalDistance) {
    const Array<std::string> stops = routes::getStopList(shuttleNumber, isIncoming);
    if (path.empty()) {
        return false;
    }

    routeStops.clear();
    totalDistance = 0.0;
    for (int i = 0; i < path.size(); ++i) {
        if (path[i] < 0 || path[i] >= stops.size()) {
            return false;
        }
        if (i > 0) {
            const double distance = edgeDistance(shuttleNumber, isIncoming,
                                                 path[i - 1], path[i]);
            if (distance <= 0.0) {
                return false;
            }
            routeStops += " -> ";
            totalDistance += distance;
        }
        routeStops += "[" + stops[path[i]] + "]";
    }
    return true;
}

void addSchedule(Shuttle& shuttle, const char* direction, const char* time) {
    Schedule schedule;
    schedule.direction = direction;
    schedule.departureTime = time;
    shuttle.schedules.push(schedule);
}

}

namespace routes {

void initialize() {
    if (ready) {
        return;
    }

    // ========================================================
    // 1. SAN PEDRO (Shuttle 1)
    // ========================================================
    // Pickup: Astrodome -> Pacita 1 -> Chrysanthemum -> Rosario -> Southwoods -> MCL
    spPickupStops.push("Astrodome");      // 0
    spPickupStops.push("Pacita 1");       // 1
    spPickupStops.push("Chrysanthemum");  // 2
    spPickupStops.push("Rosario");        // 3
    spPickupStops.push("Southwoods");     // 4
    spPickupStops.push("MCL");            // 5

    spPickupGraph.addEdge(0, 1);
    spPickupGraph.addEdge(1, 2);
    spPickupGraph.addEdge(2, 3);
    spPickupGraph.addEdge(3, 4);
    spPickupGraph.addEdge(4, 5);

    // Drop-off: MCL -> Halang -> Southwoods/Rosario -> Chrysanthemum -> Pacita 1 -> Macaria/Labasan -> Galleria -> United
    spDropoffStops.push("MCL");               // 0
    spDropoffStops.push("Halang");            // 1
    spDropoffStops.push("SouthwoodsMall");    // 2
    spDropoffStops.push("Rosario");           // 3
    spDropoffStops.push("Chrysanthemum");     // 4
    spDropoffStops.push("Pacita 1");          // 5
    spDropoffStops.push("Macaria");           // 6
    spDropoffStops.push("Labasan");           // 7
    spDropoffStops.push("Galleria");          // 8
    spDropoffStops.push("SanPedro(United)");  // 9

    spDropoffGraph.addEdge(0, 1);
    spDropoffGraph.addEdge(1, 2);
    spDropoffGraph.addEdge(1, 3);
    spDropoffGraph.addEdge(2, 3);
    spDropoffGraph.addEdge(3, 4);
    spDropoffGraph.addEdge(4, 5);
    spDropoffGraph.addEdge(5, 6);
    spDropoffGraph.addEdge(5, 7);
    spDropoffGraph.addEdge(6, 7);
    spDropoffGraph.addEdge(7, 8);
    spDropoffGraph.addEdge(8, 9);

    Shuttle sanPedro;
    sanPedro.number = 1;
    sanPedro.name = "San Pedro Shuttle";
    sanPedro.route = "San Pedro (Pacita/United) - MCL";
    for (int i = 0; i < spPickupStops.size(); ++i) {
        sanPedro.stopIds.push(i);
    }
    addSchedule(sanPedro, "Incoming (Pickup to MCL)", "6:30 AM");
    addSchedule(sanPedro, "Incoming (Pickup to MCL)", "7:30 AM");
    addSchedule(sanPedro, "Outgoing (MCL Drop-off)", "4:30 PM");
    addSchedule(sanPedro, "Outgoing (MCL Drop-off)", "6:00 PM");
    catalog.push(sanPedro);

    // ========================================================
    // 2. STA. ROSA / PASEO (Shuttle 2)
    // ========================================================
    staRosaStops.push("MMCL");      // 0
    staRosaStops.push("PRAMANA");   // 1
    staRosaStops.push("ZADIA");     // 2
    staRosaStops.push("SOLEN");     // 3
    staRosaStops.push("PASEO");     // 4
    staRosaStops.push("BELAIR");    // 5
    staRosaStops.push("DON_JOSE");  // 6

    staRosaGraph.addEdge(0, 1);
    staRosaGraph.addEdge(1, 2);
    staRosaGraph.addEdge(1, 3);
    staRosaGraph.addEdge(1, 4);
    staRosaGraph.addEdge(1, 5);
    staRosaGraph.addEdge(2, 3);
    staRosaGraph.addEdge(2, 4);
    staRosaGraph.addEdge(3, 4);
    staRosaGraph.addEdge(3, 5);
    staRosaGraph.addEdge(4, 6);
    staRosaGraph.addEdge(5, 6);

    Shuttle staRosa;
    staRosa.number = 2;
    staRosa.name = "Sta. Rosa (Paseo) Shuttle";
    staRosa.route = "Paseo / Nuvali - MMCL";
    for (int i = 0; i < staRosaStops.size(); ++i) {
        staRosa.stopIds.push(i);
    }
    addSchedule(staRosa, "Incoming (Pickup to MMCL)", "6:45 AM");
    addSchedule(staRosa, "Incoming (Pickup to MMCL)", "7:45 AM");
    addSchedule(staRosa, "Outgoing (MMCL Drop-off)", "4:45 PM");
    addSchedule(staRosa, "Outgoing (MMCL Drop-off)", "6:15 PM");
    catalog.push(staRosa);

    // ========================================================
    // 3. CALAMBA (Shuttle 3)
    // ========================================================
    calambaStops.push("MMCL");   // 0
    calambaStops.push("BATINO"); // 1
    calambaStops.push("REAL");   // 2
    calambaStops.push("SM");     // 3

    calambaGraph.addEdge(0, 1);
    calambaGraph.addEdge(1, 2);
    calambaGraph.addEdge(2, 3);

    Shuttle calamba;
    calamba.number = 3;
    calamba.name = "Calamba Shuttle";
    calamba.route = "SM Calamba / Real / Batino - MMCL";
    for (int i = 0; i < calambaStops.size(); ++i) {
        calamba.stopIds.push(i);
    }
    addSchedule(calamba, "Incoming (Pickup to MMCL)", "6:45 AM");
    addSchedule(calamba, "Incoming (Pickup to MMCL)", "8:00 AM");
    addSchedule(calamba, "Outgoing (MMCL Drop-off)", "4:30 PM");
    addSchedule(calamba, "Outgoing (MMCL Drop-off)", "6:00 PM");
    catalog.push(calamba);

    // Global stop names fallback
    for (int i = 0; i < spPickupStops.size(); ++i) {
        names.push(spPickupStops[i]);
    }

    ready = true;
}

const Array<Shuttle>& shuttles() {
    initialize();
    return catalog;
}

const Shuttle* findShuttle(int shuttleNumber) {
    initialize();
    for (int i = 0; i < catalog.size(); ++i) {
        if (catalog[i].number == shuttleNumber) {
            return &catalog[i];
        }
    }
    return nullptr;
}

const Graph& network() {
    initialize();
    return spPickupGraph;
}

const Array<std::string>& stopNames() {
    initialize();
    return names;
}

void printPickupSequence() {
    initialize();
    std::cout << "Available Regional Shuttles:\n"
              << "1. San Pedro Shuttle (Pacita / United)\n"
              << "2. Sta. Rosa (Paseo / Nuvali) Shuttle\n"
              << "3. Calamba Shuttle (SM Calamba / Real / Batino)\n";
}

void printShuttlePath(int shuttleNumber) {
    initialize();
    const Shuttle* shuttle = findShuttle(shuttleNumber);
    if (shuttle == nullptr) {
        std::cout << "No shuttle matches that number.\n";
        return;
    }
    std::cout << shuttle->name << " (" << shuttle->route << ")\n";
}

void printBfsRoute(int shuttleNumber) {
    initialize();
    if (shuttleNumber == 1) {
        Array<int> path = spPickupGraph.bfsPath(0, 5);
        printDynamicRoute(1, true, path);
    } else if (shuttleNumber == 2) {
        Array<int> path = staRosaGraph.bfsPath(4, 0);
        printDynamicRoute(2, true, path);
    } else if (shuttleNumber == 3) {
        Array<int> path = calambaGraph.bfsPath(3, 0);
        printDynamicRoute(3, true, path);
    }
}

Array<std::string> getStopList(int shuttleNumber, bool isIncoming) {
    initialize();
    if (shuttleNumber == 1) {
        return isIncoming ? spPickupStops : spDropoffStops;
    } else if (shuttleNumber == 2) {
        return staRosaStops;
    } else {
        return calambaStops;
    }
}

Array<int> computeDynamicPath(int shuttleNumber, bool isIncoming, int chosenStopIndex) {
    initialize();

    if (shuttleNumber == 1) {
        // San Pedro
        if (isIncoming) {
            // Chosen pickup -> MCL (Node 5)
            return spPickupGraph.bfsPath(chosenStopIndex, 5);
        } else {
            // MCL (Node 0) -> Chosen drop-off
            return spDropoffGraph.bfsPath(0, chosenStopIndex);
        }
    } else if (shuttleNumber == 2) {
        // Sta. Rosa
        if (isIncoming) {
            // Chosen pickup -> MMCL (Node 0)
            return staRosaGraph.bfsPath(chosenStopIndex, 0);
        } else {
            // MMCL (Node 0) -> Chosen drop-off
            return staRosaGraph.bfsPath(0, chosenStopIndex);
        }
    } else {
        // Calamba
        if (isIncoming) {
            // Chosen pickup -> MMCL (Node 0)
            return calambaGraph.bfsPath(chosenStopIndex, 0);
        } else {
            // MMCL (Node 0) -> Chosen drop-off
            return calambaGraph.bfsPath(0, chosenStopIndex);
        }
    }
}

void printDynamicRoute(int shuttleNumber, bool isIncoming, const Array<int>& path) {
    initialize();
    std::string routeStops;
    double totalDistance = 0.0;
    if (!formatPathDetails(shuttleNumber, isIncoming, path, routeStops, totalDistance)) {
        std::cout << "(No connecting path available)\n";
        return;
    }
    std::cout << "\n>> Updated Dynamic Route Path (BFS):\n   " << routeStops;
    const std::ios::fmtflags previousFlags = std::cout.flags();
    const std::streamsize previousPrecision = std::cout.precision();
    std::cout << "\nTotal Path Distance: " << std::fixed << std::setprecision(1)
              << totalDistance << " km\n";
    std::cout.flags(previousFlags);
    std::cout.precision(previousPrecision);
}

bool scheduledRouteDetails(int shuttleNumber, int scheduleNumber,
                           std::string& routeStops, double& totalDistance) {
    initialize();
    bool isIncoming = false;
    Array<int> path;
    if (!buildScheduledPath(shuttleNumber, scheduleNumber, isIncoming, path)) {
        return false;
    }
    return formatPathDetails(shuttleNumber, isIncoming, path,
                             routeStops, totalDistance);
}

}