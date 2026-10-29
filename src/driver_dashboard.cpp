#include "console.h"
#include "dashboards.h"
#include "ds/algo.h"
#include "routes.h"
#include "store.h"

#include <iostream>
#include <string>

using namespace std;

namespace {

void displayTrip(const Trip& trip, const Store& store) {
    cout << tripDescription(trip) << " | Passengers: "
         << activePassengerCount(store, trip.id) << '/' << trip.capacity << '\n';
}

void displayPassenger(const Reservation& reservation) {
    cout << "- " << reservation.studentId
         << " | Seat " << reservation.seatNumber
         << " | " << reservation.paymentMethod;
    if (isOnlinePayment(reservation.paymentMethod)) {
        cout << " ref " << (reservation.referenceNumber.empty()
                                ? "(missing)"
                                : reservation.referenceNumber);
    }
    cout << " | payment " << reservation.paymentStatus
         << " | " << reservation.status
         << " | " << reservation.ticketCode << '\n';
}

void sortPassengers(Array<Reservation*>& passengers, int mode) {
    if (mode == 2) {
        quickSort(passengers, [](const Reservation* left, const Reservation* right) {
            return left->seatNumber < right->seatNumber;
        });
    } else if (mode == 3) {
        quickSort(passengers, [](const Reservation* left, const Reservation* right) {
            if (left->status != right->status) {
                return left->status < right->status;
            }
            return left->studentId < right->studentId;
        });
    } else {
        quickSort(passengers, [](const Reservation* left, const Reservation* right) {
            return left->studentId < right->studentId;
        });
    }
}

int selectAssignedTrip(Store& store, const string& driver, const string& prompt,
                       bool openOnly) {
    Array<Trip*> assigned;
    collectAssignedTrips(store, driver, openOnly, assigned);
    quickSort(assigned, [](const Trip* left, const Trip* right) {
        if (left->date != right->date) {
            return left->date < right->date;
        }
        return left->id < right->id;
    });
    if (assigned.empty()) {
        cout << "You have no matching assigned trips.\n";
        return 0;
    }
    cout << '\n';
    for (int i = 0; i < assigned.size(); ++i) {
        cout << "- " << tripDescription(*assigned[i]) << '\n';
    }
    int tripId;
    if (!readNumber(prompt, tripId) || tripId == 0) {
        return 0;
    }
    for (int i = 0; i < assigned.size(); ++i) {
        if (assigned[i]->id == tripId) {
            return tripId;
        }
    }
    cout << "That trip is not in your assigned trips.\n";
    return 0;
}

void showDashboard(const string& driver, Store& store) {
    int assignedCount = 0;
    int passengerCount = 0;
    int boardedCount = 0;
    int pendingPayments = 0;
    int waitlisted = 0;
    Array<Trip*> assigned;
    collectAssignedTrips(store, driver, true, assigned);
    for (int i = 0; i < assigned.size(); ++i) {
        Trip* trip = assigned[i];
        ++assignedCount;
        for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
            const Reservation& reservation = node->value;
            if (reservation.tripId != trip->id || !occupiesSeat(reservation)) {
                continue;
            }
            ++passengerCount;
            if (reservation.status == "Boarded") {
                ++boardedCount;
            }
            if (reservation.paymentStatus == "Pending") {
                ++pendingPayments;
            }
        }
    }
    store.waitlist.forEach([&](const WaitlistEntry& entry) {
        const Trip* trip = findTrip(store, entry.tripId);
        if (trip != nullptr && trip->driver == driver) {
            ++waitlisted;
        }
    });

    cout << "\n=== Driver Dashboard ===\n"
         << "Driver: " << driver << '\n'
         << "Assigned trips: " << assignedCount << '\n'
         << "Passengers: " << passengerCount << '\n'
         << "Boarded: " << boardedCount << '\n'
         << "Pending payment review: " << pendingPayments << '\n'
         << "Waitlisted riders: " << waitlisted << '\n';
}

void showAssignedTrips(const string& driver, Store& store) {
    cout << "\n=== Assigned Trips ===\n";
    Array<Trip*> assigned;
    collectAssignedTrips(store, driver, false, assigned);
    quickSort(assigned, [](const Trip* left, const Trip* right) {
        if (left->date != right->date) {
            return left->date < right->date;
        }
        return left->id < right->id;
    });
    if (assigned.empty()) {
        cout << "No trips are assigned to your account.\n";
        return;
    }
    for (int i = 0; i < assigned.size(); ++i) {
        displayTrip(*assigned[i], store);
        Array<Reservation*> passengers;
        collectTripPassengers(store, assigned[i]->id, true, passengers);
        sortPassengers(passengers, 1);
        if (passengers.empty()) {
            cout << "No passengers reserved for this trip.\n";
        } else {
            for (int p = 0; p < passengers.size(); ++p) {
                displayPassenger(*passengers[p]);
            }
        }
    }
}

void showManifest(const string& driver, Store& store) {
    const int tripId = selectAssignedTrip(
        store, driver, "Enter a trip ID (0 to return): ", true);
    if (tripId == 0) {
        return;
    }
    const Trip* trip = findTrip(store, tripId);
    Array<Reservation*> passengers;
    collectTripPassengers(store, tripId, false, passengers);
    cout << "Sort manifest by:\n1. Student ID\n2. Seat number\n3. Boarding status\n";
    int mode;
    if (!readNumber("Choose an option: ", mode)) {
        mode = 1;
    }
    sortPassengers(passengers, mode);
    cout << "\n=== Passenger Manifest: " << tripDescription(*trip) << " ===\n";
    if (passengers.empty()) {
        cout << "No passengers reserved for this trip.\n";
        return;
    }
    for (int i = 0; i < passengers.size(); ++i) {
        displayPassenger(*passengers[i]);
    }
}

void searchPassenger(const string& driver, Store& store) {
    cout << "\n1. Student ID (binary search on sorted manifest)\n"
         << "2. Payment reference (hash table)\n"
         << "3. Payment reference (binary search)\n"
         << "4. Ticket code (hash table)\n";
    int mode;
    if (!readNumber("Choose an option: ", mode)) {
        return;
    }

    if (mode == 2) {
        string reference;
        if (!readLine("Payment reference: ", reference) || reference.empty()) {
            return;
        }
        Reservation** found = store.paymentRefs.get(reference);
        if (found == nullptr) {
            cout << "No active reservation matches that reference.\n";
            return;
        }
        cout << "Found by hash lookup:\n";
        displayPassenger(**found);
        printDigitalPass(**found, store);
        return;
    }
    if (mode == 4) {
        string ticket;
        if (!readLine("Ticket code: ", ticket) || ticket.empty()) {
            return;
        }
        Reservation** found = store.tickets.get(ticket);
        if (found == nullptr) {
            cout << "No active reservation matches that ticket.\n";
            return;
        }
        cout << "Found by hash lookup:\n";
        displayPassenger(**found);
        printDigitalPass(**found, store);
        return;
    }

    const int tripId = selectAssignedTrip(
        store, driver, "Enter a trip ID (0 to return): ", true);
    if (tripId == 0) {
        return;
    }
    Array<Reservation*> passengers;
    collectTripPassengers(store, tripId, false, passengers);

    if (mode == 3) {
        string reference;
        if (!readLine("Payment reference: ", reference) || reference.empty()) {
            return;
        }
        Array<Reservation*> withRef;
        for (int i = 0; i < passengers.size(); ++i) {
            if (!passengers[i]->referenceNumber.empty()) {
                withRef.push(passengers[i]);
            }
        }
        quickSort(withRef, [](const Reservation* left, const Reservation* right) {
            return left->referenceNumber < right->referenceNumber;
        });
        const int index = binarySearch(
            withRef, reference,
            [](const Reservation* reservation) { return reservation->referenceNumber; });
        if (index < 0) {
            cout << "No reservation with that reference was found on this trip.\n";
            return;
        }
        cout << "Found by binary search:\n";
        displayPassenger(*withRef[index]);
        printDigitalPass(*withRef[index], store);
        return;
    }

    string studentId;
    if (!readLine("Student number/ID to find: ", studentId) || studentId.empty()) {
        return;
    }
    sortPassengers(passengers, 1);
    const int index = binarySearch(
        passengers, studentId,
        [](const Reservation* reservation) { return reservation->studentId; });
    if (index < 0) {
        cout << "No reservation for that student was found on this trip.\n";
        return;
    }
    cout << "Found by binary search:\n";
    displayPassenger(*passengers[index]);
    printDigitalPass(*passengers[index], store);
}

void verifyPayments(const string& driver, Store& store) {
    cout << "\n1. Verify by payment reference (hash lookup)\n"
         << "2. Log a cash payment from the pending list\n";
    int mode;
    if (!readNumber("Choose an option: ", mode)) {
        return;
    }
    const int tripId = selectAssignedTrip(
        store, driver, "Enter a trip ID (0 to return): ", true);
    if (tripId == 0) {
        return;
    }

    if (mode == 1) {
        string reference;
        if (!readLine("Enter the payment reference number: ", reference)
            || reference.empty()) {
            return;
        }
        Reservation** found = store.paymentRefs.get(reference);
        if (found == nullptr || (*found)->tripId != tripId) {
            cout << "No pending reservation on this trip matches that reference.\n";
            return;
        }
        Reservation& reservation = **found;
        if (!occupiesSeat(reservation)) {
            cout << "That reservation is no longer active.\n";
            return;
        }
        reservation.paymentStatus = "Verified";
        reservation.status = "Verified";
        if (!saveReservations(store)) {
            reservation.paymentStatus = "Pending";
            reservation.status = "Reserved";
            cout << "Payment verification could not be saved.\n";
            return;
        }
        cout << "Reference " << reference << " verified for "
             << reservation.studentId << ".\n";
        return;
    }

    while (true) {
        Array<Reservation*> pending;
        collectTripPassengers(store, tripId, true, pending);
        Array<Reservation*> cashPending;
        for (int i = 0; i < pending.size(); ++i) {
            if (pending[i]->paymentStatus == "Pending") {
                cashPending.push(pending[i]);
            }
        }
        sortPassengers(cashPending, 1);
        cout << "\nPending payment review for trip #" << tripId << ":\n";
        if (cashPending.empty()) {
            cout << "No payments are waiting for review.\n";
            return;
        }
        for (int i = 0; i < cashPending.size(); ++i) {
            cout << i + 1 << ". " << cashPending[i]->studentId << " - "
                 << cashPending[i]->paymentMethod;
            if (!cashPending[i]->referenceNumber.empty()) {
                cout << " ref " << cashPending[i]->referenceNumber;
            }
            cout << '\n';
        }
        int choice;
        if (!readNumber("Enter a passenger number to verify (0 to return): ", choice)
            || choice == 0) {
            return;
        }
        if (choice < 1 || choice > cashPending.size()) {
            cout << "No passenger matches that number.\n";
            continue;
        }
        Reservation& reservation = *cashPending[choice - 1];
        const string previousPay = reservation.paymentStatus;
        const string previousStatus = reservation.status;
        reservation.paymentStatus = "Verified";
        reservation.status = "Verified";
        if (!saveReservations(store)) {
            reservation.paymentStatus = previousPay;
            reservation.status = previousStatus;
            cout << "Payment verification could not be saved.\n";
            return;
        }
        cout << "Payment verified for " << reservation.studentId << ".\n";
    }
}

void boardingCheckIn(const string& driver, Store& store) {
    const int tripId = selectAssignedTrip(
        store, driver, "Enter a trip ID (0 to return): ", true);
    if (tripId == 0) {
        return;
    }
    Array<Reservation*> passengers;
    collectTripPassengers(store, tripId, true, passengers);
    Array<Reservation*> awaiting;
    for (int i = 0; i < passengers.size(); ++i) {
        if (passengers[i]->status != "Boarded") {
            awaiting.push(passengers[i]);
        }
    }
    sortPassengers(awaiting, 2);
    if (awaiting.empty()) {
        cout << "There are no passengers awaiting boarding check-in.\n";
        return;
    }
    for (int i = 0; i < awaiting.size(); ++i) {
        Reservation* passenger = awaiting[i];
        cout << '\n';
        displayPassenger(*passenger);
        cout << "1. Mark boarded\n2. Mark no-show (release seat)\n0. Leave unchanged\n";
        int choice;
        if (!readNumber("Choose an action: ", choice)) {
            return;
        }
        if (choice == 1) {
            if (passenger->paymentStatus != "Verified") {
                cout << "Verify payment before marking this passenger boarded.\n";
            } else {
                passenger->status = "Boarded";
            }
        } else if (choice == 2) {
            passenger->status = "No-show";
            if (!passenger->referenceNumber.empty()) {
                store.paymentRefs.remove(passenger->referenceNumber);
            }
        } else if (choice != 0) {
            cout << "Invalid action. This passenger was left unchanged.\n";
        }
    }
    if (!saveReservations(store)) {
        cout << "Boarding changes could not be saved.\n";
        return;
    }
    rebuildIndexes(store);
    promoteWaitlistForTrip(store, tripId);
    cout << "Boarding check-in changes saved. Released seats go to the waitlist.\n";
}

void manageTripStatus(const string& driver, Store& store) {
    const int tripId = selectAssignedTrip(
        store, driver, "Enter a trip ID to update (0 to return): ", true);
    if (tripId == 0) {
        return;
    }
    Trip* trip = findTrip(store, tripId);
    cout << "\n1. Mark delayed\n2. Cancel trip\n3. Conclude as completed\n0. Return\n";
    int choice;
    if (!readNumber("Choose an action: ", choice) || choice == 0) {
        return;
    }
    const string previousStatus = trip->status;
    if (choice == 1) {
        trip->status = "Delayed";
    } else if (choice == 2) {
        trip->status = "Cancelled";
        for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
            if (node->value.tripId == tripId && occupiesSeat(node->value)) {
                node->value.status = "Cancelled";
            }
        }
    } else if (choice == 3) {
        trip->status = "Completed";
    } else {
        cout << "Invalid action.\n";
        return;
    }
    if (!saveTrips(store) || !saveReservations(store)) {
        trip->status = previousStatus;
        cout << "Trip status could not be saved.\n";
        return;
    }
    rebuildIndexes(store);
    cout << "Trip #" << tripId << " is now " << trip->status << ".\n";
}

bool addTripForDriver(const string& driver, Store& store) {
    cout << "\nChoose your area of operation:\n";
    const Array<Shuttle>& shuttles = routes::shuttles();
    for (int i = 0; i < shuttles.size(); ++i) {
        cout << shuttles[i].number << ". " << shuttles[i].route
             << " (" << shuttles[i].name << ")\n";
        routes::printShuttlePath(shuttles[i].number);
    }
    int shuttleNumber;
    if (!readNumber("Choose an area/shuttle number (0 to return): ", shuttleNumber)
        || shuttleNumber == 0) {
        return false;
    }
    const Shuttle* shuttle = routes::findShuttle(shuttleNumber);
    if (shuttle == nullptr) {
        cout << "No area matches that number.\n";
        return false;
    }

    string date;
    if (!readLine("Enter trip date (YYYY-MM-DD): ", date)) {
        return false;
    }
    const string currentDate = today();
    if (!isValidDate(date) || (!currentDate.empty() && date < currentDate)) {
        cout << "Enter a valid date that is today or later.\n";
        return false;
    }

    cout << "\nSchedules for " << shuttle->name << ":\n";
    for (int i = 0; i < shuttle->schedules.size(); ++i) {
        cout << i + 1 << ". " << shuttle->schedules[i].direction
             << " - " << shuttle->schedules[i].departureTime << '\n';
    }
    int scheduleNumber;
    if (!readNumber("Choose a schedule number: ", scheduleNumber)) {
        return false;
    }
    if (scheduleNumber < 1 || scheduleNumber > shuttle->schedules.size()) {
        cout << "No schedule matches that number.\n";
        return false;
    }

    for (int i = 0; i < store.trips.size(); ++i) {
        const Trip& trip = store.trips[i];
        if (trip.shuttleNumber == shuttleNumber && trip.scheduleNumber == scheduleNumber
            && trip.date == date && isOpenTrip(trip)) {
            cout << "That shuttle schedule already has an active trip on that date.\n";
            return false;
        }
    }

    int capacity;
    if (!readNumber("Enter the shuttle seat capacity: ", capacity) || capacity < 1) {
        cout << "Seat capacity must be a positive number.\n";
        return false;
    }

    int nextId = 1;
    for (int i = 0; i < store.trips.size(); ++i) {
        if (store.trips[i].id + 1 > nextId) {
            nextId = store.trips[i].id + 1;
        }
    }
    Trip trip;
    trip.id = nextId;
    trip.driver = driver;
    trip.date = date;
    trip.shuttleNumber = shuttleNumber;
    trip.scheduleNumber = scheduleNumber;
    trip.capacity = capacity;
    trip.status = "Scheduled";
    if (!appendTrip(store, trip)) {
        cout << "Trip was not assigned because it could not be saved.\n";
        return false;
    }
    cout << "Assigned trip #" << nextId << " for " << driver << ": "
         << tripDescription(trip) << ".\n";
    return true;
}

void showTripHistory(const string& driver, Store& store) {
    cout << "\n=== Completed Trip History ===\n";
    Array<Trip*> completed;
    for (int i = 0; i < store.trips.size(); ++i) {
        if (store.trips[i].driver == driver && store.trips[i].status == "Completed") {
            completed.push(&store.trips[i]);
        }
    }
    quickSort(completed, [](const Trip* left, const Trip* right) {
        if (left->date != right->date) {
            return left->date > right->date;
        }
        return left->id > right->id;
    });
    if (completed.empty()) {
        cout << "No trips have been completed yet.\n";
        return;
    }
    for (int i = 0; i < completed.size(); ++i) {
        displayTrip(*completed[i], store);
        int boarded = 0;
        for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
            if (node->value.tripId == completed[i]->id
                && node->value.status == "Boarded") {
                ++boarded;
            }
        }
        cout << "Boarded passengers: " << boarded << '\n';
    }
}

void showShuttleMonitoring(Store& store) {
    const Array<Shuttle>& shuttles = routes::shuttles();
    Array<int> availableSeats;
    Array<int> usedArea;
    for (int i = 0; i < shuttles.size(); ++i) {
        availableSeats.push(0);
        usedArea.push(0);
    }
    const string currentDate = today();
    int availableShuttles = 0;
    for (int i = 0; i < store.trips.size(); ++i) {
        const Trip& trip = store.trips[i];
        if (!isOpenTrip(trip) || (!currentDate.empty() && trip.date < currentDate)) {
            continue;
        }
        const int reserved = activePassengerCount(store, trip.id);
        if (reserved >= trip.capacity) {
            continue;
        }
        const int area = trip.shuttleNumber - 1;
        if (usedArea[area] == 0) {
            usedArea[area] = 1;
            ++availableShuttles;
        }
        availableSeats[area] += trip.capacity - reserved;
    }

    cout << "\n=== Shuttle Monitoring ===\n"
         << "Available shuttles: " << availableShuttles
         << " of " << shuttles.size() << " shuttle areas with open seats\n";
    routes::printPickupSequence();
    cout << "Available seats by area:\n";
    for (int i = 0; i < shuttles.size(); ++i) {
        cout << "- " << shuttles[i].route << ": " << availableSeats[i]
             << " seats across scheduled trips\n";
    }
}

void showWaitlist(const string& driver, Store& store) {
    cout << "\n=== Trip Waitlists (FIFO) ===\n";
    int count = 0;
    int position = 0;
    store.waitlist.forEach([&](const WaitlistEntry& entry) {
        ++position;
        const Trip* trip = findTrip(store, entry.tripId);
        if (trip == nullptr || trip->driver != driver) {
            return;
        }
        cout << position << ". Trip #" << entry.tripId << " | "
             << entry.studentId << " | " << entry.paymentMethod;
        if (!entry.referenceNumber.empty()) {
            cout << " | ref " << entry.referenceNumber;
        }
        cout << '\n';
        ++count;
    });
    if (count == 0) {
        cout << "No waitlisted students for your trips.\n";
    }
}

void runCutoff(Store& store) {
    const int released = applyCutoffAndReallocate(store);
    if (released == 0) {
        cout << "No unverified or unclaimed reservations are at the "
             << kCutoffMinutes << "-minute cutoff yet.\n";
        return;
    }
    cout << released
         << " reservation(s) were cancelled or marked no-show. "
            "Freed seats were offered to the waitlist.\n";
}

}

void driverDashboard(const string& driverUsername) {
    Store store;
    if (!loadStore(store)) {
        cout << "Could not load driver trip data. Logging out.\n";
        return;
    }
    const int released = applyCutoffAndReallocate(store);
    if (released > 0) {
        cout << released
             << " unverified or unclaimed reservation(s) were released at cutoff.\n";
    }

    string choice;
    while (true) {
        cout << "\n=== Driver Menu ===\n"
             << "1. Dashboard\n"
             << "2. Assigned Trips\n"
             << "3. Passenger Manifest\n"
             << "4. Search Student (binary search)\n"
             << "5. Payment Verification\n"
             << "6. Boarding Check-in\n"
             << "7. Cutoff / No-Show Manager\n"
             << "8. Waitlist\n"
             << "9. Trip History\n"
             << "10. Plan/Assign a Trip\n"
             << "11. Shuttle Monitoring\n"
             << "12. Update Trip Status\n"
             << "13. Logout\n"
             << "Choose an option: ";
        if (!getline(cin, choice)) {
            return;
        }

        if (choice == "1") {
            showDashboard(driverUsername, store);
        } else if (choice == "2") {
            showAssignedTrips(driverUsername, store);
        } else if (choice == "3") {
            showManifest(driverUsername, store);
        } else if (choice == "4") {
            searchPassenger(driverUsername, store);
        } else if (choice == "5") {
            verifyPayments(driverUsername, store);
        } else if (choice == "6") {
            boardingCheckIn(driverUsername, store);
        } else if (choice == "7") {
            runCutoff(store);
        } else if (choice == "8") {
            showWaitlist(driverUsername, store);
        } else if (choice == "9") {
            showTripHistory(driverUsername, store);
        } else if (choice == "10") {
            addTripForDriver(driverUsername, store);
        } else if (choice == "11") {
            showShuttleMonitoring(store);
        } else if (choice == "12") {
            manageTripStatus(driverUsername, store);
        } else if (choice == "13") {
            cout << "Logged out. Returning to the main menu.\n";
            return;
        } else {
            cout << "Invalid option. Choose 1 through 13.\n";
        }
    }
}
