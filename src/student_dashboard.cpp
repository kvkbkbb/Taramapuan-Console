#include "console.h"
#include "dashboards.h"
#include "ds/algo.h"
#include "routes.h"
#include "store.h"

#include <iostream>
#include <string>

using namespace std;

namespace {

bool hasFutureDate(const Trip& trip) {
    const string currentDate = today();
    return currentDate.empty() || trip.date >= currentDate;
}

bool reservationIsUpcoming(const Reservation& reservation, const Store& store) {
    if (!occupiesSeat(reservation) && reservation.status != "Waitlisted") {
        return false;
    }
    if (reservation.tripId == 0) {
        return true;
    }
    const Trip* trip = findTrip(store, reservation.tripId);
    return trip != nullptr && isOpenTrip(*trip) && hasFutureDate(*trip);
}

void displayReservation(const Reservation& reservation, const Store& store) {
    if (reservation.tripId == 0) {
        const Shuttle* shuttle = routes::findShuttle(reservation.shuttleNumber);
        const Schedule& schedule = shuttle->schedules[reservation.scheduleNumber - 1];
        cout << shuttle->name << ", " << schedule.direction << " at "
             << schedule.departureTime << " (legacy); ";
    } else {
        const Trip* trip = findTrip(store, reservation.tripId);
        if (trip == nullptr) {
            cout << "Trip #" << reservation.tripId << "; ";
        } else {
            cout << tripDescription(*trip) << "; seat "
                 << reservation.seatNumber << "; ";
        }
    }
    cout << reservation.paymentMethod << " - " << reservation.paymentStatus
         << "; " << reservation.status;
    if (!reservation.ticketCode.empty()) {
        cout << "; ticket " << reservation.ticketCode;
    }
    cout << '\n';
}

bool choosePayment(Store& store, string& method, string& reference) {
    cout << "Choose a payment method:\n"
         << "1. Cash\n"
         << "2. GCash\n"
         << "3. Maya\n"
         << "4. GoTyme\n";
    int choice;
    if (!readNumber("Choose an option: ", choice)) {
        return false;
    }
    if (choice == 1) {
        method = "Cash";
        reference.clear();
        return true;
    }
    if (choice == 2) {
        method = "GCash";
    } else if (choice == 3) {
        method = "Maya";
    } else if (choice == 4) {
        method = "GoTyme";
    } else {
        cout << "Invalid payment method.\n";
        return false;
    }
    if (!readLine("Enter the online payment reference number: ", reference)
        || reference.empty()) {
        cout << "A reference number is required for online payment.\n";
        return false;
    }
    if (store.paymentRefs.contains(reference)) {
        cout << "That reference number is already logged.\n";
        return false;
    }
    bool waitlistDuplicate = false;
    store.waitlist.forEach([&](const WaitlistEntry& entry) {
        if (entry.referenceNumber == reference) {
            waitlistDuplicate = true;
        }
    });
    if (waitlistDuplicate) {
        cout << "That reference number is already on a waitlist request.\n";
        return false;
    }
    return true;
}

void showDashboard(const string& studentId, Store& store) {
    cout << "\n=== Student Dashboard ===\n"
         << "Student number/ID: " << studentId << "\n\n"
         << "Upcoming reservations:\n";
    int upcomingCount = 0;
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        if (node->value.studentId == studentId
            && reservationIsUpcoming(node->value, store)) {
            cout << "- ";
            displayReservation(node->value, store);
            ++upcomingCount;
        }
    }
    if (upcomingCount == 0) {
        cout << "No upcoming reservations.\n";
    }

    cout << "\nWaitlist:\n";
    int waiting = 0;
    int position = 0;
    store.waitlist.forEach([&](const WaitlistEntry& entry) {
        ++position;
        if (entry.studentId == studentId) {
            cout << "- Position " << position << " for trip #" << entry.tripId
                 << " (" << entry.paymentMethod << ")\n";
            ++waiting;
        }
    });
    if (waiting == 0) {
        cout << "You are not on a waitlist.\n";
    }

    cout << "\nPending payments:\n";
    int pendingCount = 0;
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        if (node->value.studentId == studentId
            && reservationIsUpcoming(node->value, store)
            && node->value.paymentStatus == "Pending") {
            cout << "- ";
            displayReservation(node->value, store);
            ++pendingCount;
        }
    }
    if (pendingCount == 0) {
        cout << "No pending payments.\n";
    }

    cout << "\nOpen trips:\n";
    int tripCount = 0;
    for (int i = 0; i < store.trips.size(); ++i) {
        const Trip& trip = store.trips[i];
        if (isOpenTrip(trip) && hasFutureDate(trip)) {
            const int reserved = activePassengerCount(store, trip.id);
            cout << "- " << tripDescription(trip) << " | "
                 << (trip.capacity - reserved) << " seats open\n";
            ++tripCount;
        }
    }
    if (tripCount == 0) {
        cout << "No trips have been scheduled yet.\n";
    }
}

void showSchedules(Store& store) {
    cout << "\n=== Shuttle Schedules ===\n";
    routes::printPickupSequence();
    cout << '\n';
    int count = 0;
    for (int i = 0; i < store.trips.size(); ++i) {
        const Trip& trip = store.trips[i];
        if (!isOpenTrip(trip) || !hasFutureDate(trip)) {
            continue;
        }
        const int reserved = activePassengerCount(store, trip.id);
        cout << "- " << tripDescription(trip) << " | "
             << reserved << '/' << trip.capacity << " seats filled\n";
        routes::printShuttlePath(trip.shuttleNumber);
        ++count;
    }
    if (count == 0) {
        cout << "No trips have been scheduled yet.\n";
    }
}

void reserveSeatWithDynamicPath(const std::string& studentId, Store& store) {
    routes::initialize();

    std::cout << "\n========================================\n"
              << "       SELECT SHUTTLE AREA & ROUTE\n"
              << "========================================\n";

    // 1. Choose Shuttle Area
    const Array<Shuttle>& shuttles = routes::shuttles();
    for (int i = 0; i < shuttles.size(); ++i) {
        std::cout << shuttles[i].number << ". " << shuttles[i].name
                  << " (" << shuttles[i].route << ")\n";
    }

    int shuttleChoice = 0;
    if (!readNumber("Choose your shuttle (1-3, 0 to cancel): ", shuttleChoice) || shuttleChoice == 0) {
        return;
    }
    if (shuttleChoice < 1 || shuttleChoice > shuttles.size()) {
        std::cout << "Invalid shuttle choice.\n";
        return;
    }

    // 2. Choose Direction
    std::cout << "\nSelect trip direction:\n"
              << "1. Incoming to School (Morning Pickup)\n"
              << "2. Outgoing from School (Afternoon Drop-off)\n";
    int dirChoice = 0;
    if (!readNumber("Choose direction: ", dirChoice) || (dirChoice != 1 && dirChoice != 2)) {
        std::cout << "Invalid direction.\n";
        return;
    }
    bool isIncoming = (dirChoice == 1);

    // 3. Choose Pickup or Drop-off Stop
    Array<std::string> stops = routes::getStopList(shuttleChoice, isIncoming);
    std::cout << (isIncoming ? "\nChoose your PICKUP point:\n" 
                             : "\nChoose your DROP-OFF point:\n");

    Array<int> selectableIndices;
    int displayIndex = 1;
    for (int i = 0; i < stops.size(); ++i) {
        // Hide the campus terminus from being chosen as boarding/alighting stop
        if (stops[i] != "MCL" && stops[i] != "MMCL") {
            std::cout << displayIndex << ". " << stops[i] << '\n';
            selectableIndices.push(i);
            ++displayIndex;
        }
    }

    int stopSelection = 0;
    if (!readNumber("Enter selection number: ", stopSelection)
        || stopSelection < 1 || stopSelection > selectableIndices.size()) {
        std::cout << "Invalid stop choice.\n";
        return;
    }

    int chosenStopIndex = selectableIndices[stopSelection - 1];

    // 4. Compute and Print Dynamic Shortest Path via BFS
    Array<int> calculatedPath = routes::computeDynamicPath(shuttleChoice, isIncoming, chosenStopIndex);
    routes::printDynamicRoute(shuttleChoice, isIncoming, calculatedPath);

    // 5. Filter and proceed to seat reservation for this specific shuttle
    std::cout << "\nProceeding to available scheduled trips for this route...\n";
    Array<Trip*> matchingTrips;
    for (int i = 0; i < store.trips.size(); ++i) {
        if (store.trips[i].shuttleNumber == shuttleChoice
            && isOpenTrip(store.trips[i]) && hasFutureDate(store.trips[i])
            && !tripAtCutoff(store.trips[i])) {
            matchingTrips.push(&store.trips[i]);
        }
    }

    if (matchingTrips.empty()) {
        std::cout << "No scheduled trips currently available for this route. Returning.\n";
        return;
    }

    for (int i = 0; i < matchingTrips.size(); ++i) {
        std::cout << i + 1 << ". " << tripDescription(*matchingTrips[i])
                  << " | " << (matchingTrips[i]->capacity - activePassengerCount(store, matchingTrips[i]->id))
                  << " seats available\n";
    }

    int tripSelection = 0;
    if (!readNumber("Select trip number (0 to cancel): ", tripSelection) || tripSelection == 0) {
        return;
    }
    if (tripSelection < 1 || tripSelection > matchingTrips.size()) {
        std::cout << "Invalid trip selection.\n";
        return;
    }

    int targetTripId = matchingTrips[tripSelection - 1]->id;
    if (studentHasTripHold(store, studentId, targetTripId)) {
        std::cout << "You already have a reservation or waitlist hold for that trip.\n";
        return;
    }

    std::string paymentMethod, reference;
    if (!choosePayment(store, paymentMethod, reference)) {
        return;
    }

    ReservationRequest request;
    request.studentId = studentId;
    request.tripId = targetTripId;
    request.paymentMethod = paymentMethod;
    request.referenceNumber = reference;

    std::string outcome;
    if (submitReservationRequest(store, request, outcome)) {
        if (outcome == "WAITLIST") {
            std::cout << "Trip full. Added to FIFO waitlist.\n";
        } else {
            std::cout << "Seat successfully reserved!\n";
            for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
                if (node->value.studentId == studentId && node->value.tripId == targetTripId
                    && occupiesSeat(node->value)) {
                    printDigitalPass(node->value, store);
                }
            }
        }
    } else {
        std::cout << (outcome.empty() ? "Reservation failed." : outcome) << '\n';
    }
}

void showPasses(const string& studentId, Store& store) {
    Array<Reservation*> passes;
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        if (node->value.studentId == studentId && occupiesSeat(node->value)) {
            passes.push(&node->value);
        }
    }
    if (passes.empty()) {
        cout << "You have no active digital passes.\n";
        return;
    }
    quickSort(passes, [](const Reservation* left, const Reservation* right) {
        if (left->date != right->date) {
            return left->date < right->date;
        }
        return left->tripId < right->tripId;
    });
    for (int i = 0; i < passes.size(); ++i) {
        printDigitalPass(*passes[i], store);
    }
}

void manageReservations(const string& studentId, Store& store) {
    cout << "\n=== My Reservations ===\n";
    Array<Reservation*> mine;
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        if (node->value.studentId == studentId && occupiesSeat(node->value)) {
            mine.push(&node->value);
        }
    }
    quickSort(mine, [](const Reservation* left, const Reservation* right) {
        if (left->date != right->date) {
            return left->date < right->date;
        }
        return left->tripId < right->tripId;
    });
    if (mine.empty()) {
        cout << "You have no active reservations.\n";
    } else {
        for (int i = 0; i < mine.size(); ++i) {
            cout << i + 1 << ". ";
            displayReservation(*mine[i], store);
        }
        int choice;
        if (!readNumber("Enter a number to cancel (0 to return): ", choice)
            || choice == 0) {
            return;
        }
        if (choice < 1 || choice > mine.size()) {
            cout << "No reservation matches that number.\n";
            return;
        }
        Reservation* reservation = mine[choice - 1];
        reservation->status = "Cancelled";
        if (!reservation->referenceNumber.empty()) {
            store.paymentRefs.remove(reservation->referenceNumber);
        }
        if (!saveReservations(store)) {
            reservation->status = "Reserved";
            cout << "Cancellation could not be saved.\n";
            return;
        }
        cout << "Reservation cancelled. The seat is released to the waitlist.\n";
        promoteWaitlistForTrip(store, reservation->tripId);
    }
}

void updatePaymentMethod(const string& studentId, Store& store) {
    StudentProfile* profile = store.studentDirectory.get(studentId);
    cout << "\nProfile payment method: "
         << (profile == nullptr ? "Not set" : profile->paymentMethod) << '\n';
    string method;
    string reference;
    if (!choosePayment(store, method, reference)) {
        return;
    }
    upsertProfile(store, studentId, method);
    if (!saveProfiles(store)) {
        cout << "Profile payment method could not be saved.\n";
        return;
    }
    cout << "Profile payment method: " << method << '\n';
}

}

void studentDashboard(const string& studentId) {
    Store store;
    if (!loadStore(store)) {
        cout << "Could not load student dashboard data. Logging out.\n";
        return;
    }
    const int released = applyCutoffAndReallocate(store);
    if (released > 0) {
        cout << released
             << " unverified or unclaimed reservation(s) were released at cutoff.\n";
    }

    string choice;
    while (true) {
        cout << "\n=== Student Menu ===\n"
             << "1. Student Dashboard\n"
             << "2. Reserve a Seat / Join Waitlist\n"
             << "3. Shuttle Schedules & Seat Counters\n"
             << "4. Digital Pass\n"
             << "5. My Reservations\n"
             << "6. Campus Route Map\n"
             << "7. Payment Details\n"
             << "8. Logout\n"
             << "Choose an option: ";
        if (!getline(cin, choice)) {
            return;
        }

        if (choice == "1") {
            showDashboard(studentId, store);
        } else if (choice == "2") {
            reserveSeatWithDynamicPath(studentId, store);
        } else if (choice == "3") {
            showSchedules(store);
        } else if (choice == "4") {
            showPasses(studentId, store);
        } else if (choice == "5") {
            manageReservations(studentId, store);
        } else if (choice == "6") {
            cout << '\n';
            routes::printPickupSequence();
            const Array<Shuttle>& shuttles = routes::shuttles();
            for (int i = 0; i < shuttles.size(); ++i) {
                routes::printShuttlePath(shuttles[i].number);
            }
        } else if (choice == "7") {
            updatePaymentMethod(studentId, store);
        } else if (choice == "8") {
            cout << "Logged out.\n";
            return;
        } else {
            cout << "Invalid option. Choose 1 through 8.\n";
        }
    }
}
