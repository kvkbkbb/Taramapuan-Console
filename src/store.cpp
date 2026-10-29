#include "store.h"

#include "io_util.h"
#include "paths.h"
#include "routes.h"

#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

namespace {

bool parseClockParts(const string& clock, int& hour, int& minute, bool& pm) {
    istringstream parser(clock);
    char colon;
    string meridian;
    if (!(parser >> hour >> colon >> minute >> meridian) || colon != ':') {
        return false;
    }
    if (meridian == "PM" || meridian == "pm") {
        pm = true;
    } else if (meridian == "AM" || meridian == "am") {
        pm = false;
    } else {
        return false;
    }
    return hour >= 1 && hour <= 12 && minute >= 0 && minute <= 59;
}

Reservation* appendReservationRecord(Store& store, const Reservation& reservation) {
    return store.reservations.append(reservation);
}

bool loadTripsFile(Store& store, bool& requiresRouteMigration) {
    const string tripPath = paths::trips();
    ifstream file;
    if (!openOptionalInput(tripPath, file)) {
        return false;
    }
    if (!file.is_open()) {
        return true;
    }

    string line;
    size_t lineNumber = 0;
    while (getline(file, line)) {
        ++lineNumber;
        istringstream record(line);
        Trip trip;
        if (!(record >> trip.id >> quoted(trip.driver) >> trip.date
                     >> trip.shuttleNumber >> trip.scheduleNumber
                     >> trip.capacity >> quoted(trip.status))) {
            cerr << "Invalid trip record in " << tripPath << " at line "
                 << lineNumber << ".\n";
            return false;
        }
        record >> ws;
        if (!record.eof()) {
            if (!(record >> quoted(trip.routeStops) >> trip.distanceKm)) {
                cerr << "Invalid route details in " << tripPath << " at line "
                     << lineNumber << ".\n";
                return false;
            }
            record >> ws;
            if (!record.eof() || trip.routeStops.empty()
                || !isfinite(trip.distanceKm) || trip.distanceKm < 0.0) {
                cerr << "Invalid route details in " << tripPath << " at line "
                     << lineNumber << ".\n";
                return false;
            }
        } else {
            if (!routes::scheduledRouteDetails(trip.shuttleNumber, trip.scheduleNumber,
                                               trip.routeStops, trip.distanceKm)) {
                cerr << "Could not calculate route details for trip in "
                     << tripPath << " at line " << lineNumber << ".\n";
                return false;
            }
            requiresRouteMigration = true;
        }
        const Shuttle* shuttle = routes::findShuttle(trip.shuttleNumber);
        if (!record.eof() || trip.id < 1 || trip.driver.empty()
            || !isValidDate(trip.date) || shuttle == nullptr
            || trip.scheduleNumber < 1
            || trip.scheduleNumber > shuttle->schedules.size()
            || trip.capacity < 1
            || (trip.status != "Scheduled" && trip.status != "Delayed"
                && trip.status != "Cancelled" && trip.status != "Completed")) {
            cerr << "Invalid trip record in " << tripPath << " at line "
                 << lineNumber << ".\n";
            return false;
        }
        if (findTrip(store, trip.id) != nullptr) {
            cerr << "Duplicate trip ID in " << tripPath << " at line "
                 << lineNumber << ".\n";
            return false;
        }
        store.trips.push(trip);
    }
    return !file.bad();
}

bool loadReservationsFile(Store& store) {
    ifstream file;
    if (!openOptionalInput(paths::reservations(), file)) {
        return false;
    }
    if (!file.is_open()) {
        return true;
    }

    string line;
    size_t lineNumber = 0;
    while (getline(file, line)) {
        ++lineNumber;
        istringstream record(line);
        Reservation reservation;
        if (record >> quoted(reservation.studentId) >> reservation.date
                   >> reservation.tripId >> reservation.shuttleNumber
                   >> reservation.scheduleNumber
                   >> quoted(reservation.paymentMethod)
                   >> quoted(reservation.paymentStatus)
                   >> quoted(reservation.status)
                   >> quoted(reservation.referenceNumber)
                   >> quoted(reservation.ticketCode)
                   >> reservation.seatNumber) {
            record >> ws;
            if (!record.eof() || reservation.studentId.empty()
                || !isValidDate(reservation.date) || reservation.tripId < 1
                || !isValidPayment(reservation.paymentMethod)
                || (reservation.paymentStatus != "Pending"
                    && reservation.paymentStatus != "Verified")
                || (reservation.status != "Reserved"
                    && reservation.status != "Verified"
                    && reservation.status != "Boarded"
                    && reservation.status != "No-show"
                    && reservation.status != "Cancelled"
                    && reservation.status != "Expired")
                || reservation.seatNumber < 0) {
                cerr << "Invalid reservation record in " << paths::reservations()
                     << " at line " << lineNumber << ".\n";
                return false;
            }
        } else {
            record.clear();
            record.str(line);
            if (record >> quoted(reservation.studentId) >> reservation.date
                       >> reservation.tripId >> reservation.shuttleNumber
                       >> reservation.scheduleNumber
                       >> quoted(reservation.paymentMethod)
                       >> quoted(reservation.paymentStatus)
                       >> quoted(reservation.status)) {
                record >> ws;
                reservation.referenceNumber.clear();
                reservation.ticketCode = makeTicketCode(reservation);
                reservation.seatNumber = 0;
            } else {
                record.clear();
                record.str(line);
                if (!(record >> quoted(reservation.studentId)
                             >> reservation.shuttleNumber
                             >> reservation.scheduleNumber
                             >> quoted(reservation.paymentMethod))) {
                    cerr << "Invalid reservation record in " << paths::reservations()
                         << " at line " << lineNumber << ".\n";
                    return false;
                }
                record >> ws;
                reservation.date.clear();
                reservation.tripId = 0;
                reservation.paymentStatus = "Pending";
                reservation.status = "Reserved";
                reservation.referenceNumber.clear();
                reservation.ticketCode.clear();
                reservation.seatNumber = 0;
            }
        }

        const Shuttle* shuttle = routes::findShuttle(reservation.shuttleNumber);
        if (shuttle == nullptr || reservation.scheduleNumber < 1
            || reservation.scheduleNumber > shuttle->schedules.size()) {
            cerr << "Invalid shuttle or schedule in " << paths::reservations()
                 << " at line " << lineNumber << ".\n";
            return false;
        }
        if (reservation.ticketCode.empty() && reservation.tripId > 0) {
            reservation.ticketCode = makeTicketCode(reservation);
        }
        appendReservationRecord(store, reservation);
    }
    return !file.bad();
}

bool loadWaitlistFile(Store& store) {
    ifstream file;
    if (!openOptionalInput(paths::waitlist(), file)) {
        return false;
    }
    if (!file.is_open()) {
        return true;
    }

    string line;
    size_t lineNumber = 0;
    while (getline(file, line)) {
        ++lineNumber;
        istringstream record(line);
        WaitlistEntry entry;
        if (!(record >> entry.tripId >> quoted(entry.studentId)
                     >> quoted(entry.paymentMethod)
                     >> quoted(entry.referenceNumber))) {
            cerr << "Invalid waitlist record in " << paths::waitlist() << " at line "
                 << lineNumber << ".\n";
            return false;
        }
        record >> ws;
        if (!record.eof() || entry.tripId < 1 || entry.studentId.empty()
            || !isValidPayment(entry.paymentMethod)) {
            cerr << "Invalid waitlist record in " << paths::waitlist() << " at line "
                 << lineNumber << ".\n";
            return false;
        }
        store.waitlist.enqueue(entry);
    }
    return !file.bad();
}

bool loadProfilesFile(Store& store) {
    ifstream file;
    if (!openOptionalInput(paths::studentProfiles(), file)) {
        return false;
    }
    if (!file.is_open()) {
        return true;
    }

    string line;
    size_t lineNumber = 0;
    while (getline(file, line)) {
        ++lineNumber;
        istringstream record(line);
        StudentProfile profile;
        if (!(record >> quoted(profile.studentId) >> quoted(profile.paymentMethod))) {
            cerr << "Invalid profile record in " << paths::studentProfiles()
                 << " at line " << lineNumber << ".\n";
            return false;
        }
        record >> ws;
        if (!record.eof() || profile.studentId.empty()
            || !isValidPayment(profile.paymentMethod)) {
            cerr << "Invalid profile record in " << paths::studentProfiles()
                 << " at line " << lineNumber << ".\n";
            return false;
        }
        upsertProfile(store, profile.studentId, profile.paymentMethod);
    }
    return !file.bad();
}

bool reservationsMatchTrips(const Store& store) {
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        const Reservation& reservation = node->value;
        if (reservation.tripId == 0) {
            continue;
        }
        const Trip* trip = findTrip(store, reservation.tripId);
        if (trip == nullptr || trip->date != reservation.date
            || trip->shuttleNumber != reservation.shuttleNumber
            || trip->scheduleNumber != reservation.scheduleNumber) {
            cerr << "Reservation for student " << reservation.studentId
                 << " references missing or mismatched trip #"
                 << reservation.tripId << ".\n";
            return false;
        }
    }
    return true;
}

}

bool isValidDate(const string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (size_t index = 0; index < date.size(); ++index) {
        if (index != 4 && index != 7
            && (date[index] < '0' || date[index] > '9')) {
            return false;
        }
    }

    const int year = stoi(date.substr(0, 4));
    const int month = stoi(date.substr(5, 2));
    const int day = stoi(date.substr(8, 2));
    if (year < 1 || month < 1 || month > 12) {
        return false;
    }

    static const int daysPerMonth[] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
    int maxDay = daysPerMonth[month - 1];
    if (month == 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))) {
        maxDay = 29;
    }
    return day >= 1 && day <= maxDay;
}

string today() {
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    if (local == nullptr) {
        return {};
    }
    ostringstream result;
    result << put_time(local, "%Y-%m-%d");
    return result.str();
}

string nowTime() {
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    if (local == nullptr) {
        return {};
    }
    ostringstream result;
    result << put_time(local, "%I:%M %p");
    return result.str();
}

int clockToMinutes(const string& clock) {
    int hour = 0;
    int minute = 0;
    bool pm = false;
    if (!parseClockParts(clock, hour, minute, pm)) {
        return -1;
    }
    hour %= 12;
    if (pm) {
        hour += 12;
    }
    return hour * 60 + minute;
}

int currentMinutes() {
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    if (local == nullptr) {
        return -1;
    }
    return local->tm_hour * 60 + local->tm_min;
}

bool tripAtCutoff(const Trip& trip) {
    const Shuttle* shuttle = routes::findShuttle(trip.shuttleNumber);
    if (shuttle == nullptr) {
        return false;
    }
    const string currentDate = today();
    if (currentDate.empty()) {
        return false;
    }
    if (trip.date < currentDate) {
        return true;
    }
    if (trip.date > currentDate) {
        return false;
    }
    const int departure = clockToMinutes(
        shuttle->schedules[trip.scheduleNumber - 1].departureTime);
    const int now = currentMinutes();
    if (departure < 0 || now < 0) {
        return false;
    }
    return now >= departure - kCutoffMinutes;
}

bool tripHasDeparted(const Trip& trip) {
    const Shuttle* shuttle = routes::findShuttle(trip.shuttleNumber);
    if (shuttle == nullptr) {
        return false;
    }
    const string currentDate = today();
    if (currentDate.empty()) {
        return false;
    }
    if (trip.date < currentDate) {
        return true;
    }
    if (trip.date > currentDate) {
        return false;
    }
    const int departure = clockToMinutes(
        shuttle->schedules[trip.scheduleNumber - 1].departureTime);
    const int now = currentMinutes();
    return departure >= 0 && now >= 0 && now >= departure;
}

bool isValidPayment(const string& method) {
    return method == "Cash" || method == "GCash" || method == "Maya"
        || method == "GoTyme";
}

bool isOnlinePayment(const string& method) {
    return method == "GCash" || method == "Maya" || method == "GoTyme";
}

bool isActiveReservation(const Reservation& reservation) {
    return reservation.status != "No-show" && reservation.status != "Cancelled"
        && reservation.status != "Expired" && reservation.status != "Waitlisted";
}

bool isOpenTrip(const Trip& trip) {
    return trip.status == "Scheduled" || trip.status == "Delayed";
}

bool occupiesSeat(const Reservation& reservation) {
    return isActiveReservation(reservation) && reservation.tripId > 0;
}

const Trip* findTrip(const Store& store, int tripId) {
    for (int i = 0; i < store.trips.size(); ++i) {
        if (store.trips[i].id == tripId) {
            return &store.trips[i];
        }
    }
    return nullptr;
}

Trip* findTrip(Store& store, int tripId) {
    for (int i = 0; i < store.trips.size(); ++i) {
        if (store.trips[i].id == tripId) {
            return &store.trips[i];
        }
    }
    return nullptr;
}

int activePassengerCount(const Store& store, int tripId) {
    int count = 0;
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        if (node->value.tripId == tripId && occupiesSeat(node->value)) {
            ++count;
        }
    }
    return count;
}

int nextSeatNumber(const Store& store, int tripId, int capacity) {
    Array<int> used;
    for (int i = 0; i <= capacity; ++i) {
        used.push(0);
    }
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        const Reservation& reservation = node->value;
        if (reservation.tripId == tripId && occupiesSeat(reservation)
            && reservation.seatNumber >= 1 && reservation.seatNumber <= capacity) {
            used[reservation.seatNumber] = 1;
        }
    }
    for (int seat = 1; seat <= capacity; ++seat) {
        if (used[seat] == 0) {
            return seat;
        }
    }
    return 0;
}

string tripDescription(const Trip& trip) {
    const Shuttle* shuttle = routes::findShuttle(trip.shuttleNumber);
    const Schedule& schedule = shuttle->schedules[trip.scheduleNumber - 1];
    ostringstream description;
    description << "Trip #" << trip.id << " | " << trip.date << " | "
                << shuttle->name << " (" << shuttle->route << ") | "
                << schedule.direction << " at " << schedule.departureTime
                << " | " << trip.status;
    return description.str();
}

string makeTicketCode(const Reservation& reservation) {
    ostringstream code;
    code << "TM-" << reservation.tripId << "-" << reservation.studentId;
    if (reservation.seatNumber > 0) {
        code << "-S" << reservation.seatNumber;
    }
    return code.str();
}

string verificationPayload(const Reservation& reservation) {
    ostringstream payload;
    payload << reservation.ticketCode << '|' << reservation.studentId
            << "|SEAT-" << reservation.seatNumber << '|'
            << reservation.paymentMethod << '|' << reservation.referenceNumber;
    return payload.str();
}

void rebuildIndexes(Store& store) {
    store.paymentRefs.clear();
    store.tickets.clear();
    store.studentDirectory.clear();
    for (int i = 0; i < store.profiles.size(); ++i) {
        store.studentDirectory.put(store.profiles[i].studentId, store.profiles[i]);
    }
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        Reservation& reservation = node->value;
        if (!occupiesSeat(reservation)) {
            continue;
        }
        if (!reservation.referenceNumber.empty()) {
            store.paymentRefs.put(reservation.referenceNumber, &reservation);
        }
        if (!reservation.ticketCode.empty()) {
            store.tickets.put(reservation.ticketCode, &reservation);
        }
    }
}

void upsertProfile(Store& store, const string& studentId, const string& paymentMethod) {
    StudentProfile profile;
    profile.studentId = studentId;
    profile.paymentMethod = paymentMethod;
    for (int i = 0; i < store.profiles.size(); ++i) {
        if (store.profiles[i].studentId == studentId) {
            store.profiles[i].paymentMethod = paymentMethod;
            store.studentDirectory.put(studentId, profile);
            return;
        }
    }
    store.profiles.push(profile);
    store.studentDirectory.put(studentId, profile);
}

bool saveTrips(const Store& store) {
    return replaceFile(paths::trips(), [&store](ostream& file) {
        for (int i = 0; i < store.trips.size(); ++i) {
            const Trip& trip = store.trips[i];
            file << trip.id << ' ' << quoted(trip.driver) << ' '
                 << trip.date << ' ' << trip.shuttleNumber << ' '
                 << trip.scheduleNumber << ' ' << trip.capacity << ' '
                 << quoted(trip.status) << ' ' << quoted(trip.routeStops) << ' '
                 << fixed << setprecision(1) << trip.distanceKm << '\n';
        }
    });
}

bool saveReservations(const Store& store) {
    return replaceFile(paths::reservations(), [&store](ostream& file) {
        for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
            const Reservation& reservation = node->value;
            if (reservation.tripId == 0) {
                file << quoted(reservation.studentId) << ' '
                     << reservation.shuttleNumber << ' '
                     << reservation.scheduleNumber << ' '
                     << quoted(reservation.paymentMethod) << '\n';
            } else {
                file << quoted(reservation.studentId) << ' '
                     << reservation.date << ' ' << reservation.tripId << ' '
                     << reservation.shuttleNumber << ' '
                     << reservation.scheduleNumber << ' '
                     << quoted(reservation.paymentMethod) << ' '
                     << quoted(reservation.paymentStatus) << ' '
                     << quoted(reservation.status) << ' '
                     << quoted(reservation.referenceNumber) << ' '
                     << quoted(reservation.ticketCode) << ' '
                     << reservation.seatNumber << '\n';
            }
        }
    });
}

bool saveWaitlist(const Store& store) {
    return replaceFile(paths::waitlist(), [&store](ostream& file) {
        store.waitlist.forEach([&file](const WaitlistEntry& entry) {
            file << entry.tripId << ' ' << quoted(entry.studentId) << ' '
                 << quoted(entry.paymentMethod) << ' '
                 << quoted(entry.referenceNumber) << '\n';
        });
    });
}

bool saveProfiles(const Store& store) {
    return replaceFile(paths::studentProfiles(), [&store](ostream& file) {
        for (int i = 0; i < store.profiles.size(); ++i) {
            file << quoted(store.profiles[i].studentId) << ' '
                 << quoted(store.profiles[i].paymentMethod) << '\n';
        }
    });
}

bool appendTrip(Store& store, const Trip& trip) {
    Trip savedTrip = trip;
    if (!routes::scheduledRouteDetails(savedTrip.shuttleNumber,
                                       savedTrip.scheduleNumber,
                                       savedTrip.routeStops,
                                       savedTrip.distanceKm)) {
        cerr << "Could not calculate route details for trip #"
             << savedTrip.id << ".\n";
        return false;
    }

    ofstream file(paths::trips(), ios::app);
    if (!file.is_open()) {
        cerr << "Could not open " << paths::trips() << " for saving.\n";
        return false;
    }
    file << savedTrip.id << ' ' << quoted(savedTrip.driver) << ' '
         << savedTrip.date << ' ' << savedTrip.shuttleNumber << ' '
         << savedTrip.scheduleNumber << ' ' << savedTrip.capacity << ' '
         << quoted(savedTrip.status) << ' ' << quoted(savedTrip.routeStops) << ' '
         << fixed << setprecision(1) << savedTrip.distanceKm << '\n';
    file.flush();
    if (!file) {
        cerr << "Could not save trip to " << paths::trips() << ".\n";
        return false;
    }
    store.trips.push(savedTrip);
    return true;
}

bool loadStore(Store& store) {
    routes::initialize();
    bool requiresRouteMigration = false;
    if (!loadTripsFile(store, requiresRouteMigration) || !loadReservationsFile(store)
        || !loadWaitlistFile(store) || !loadProfilesFile(store)
        || !reservationsMatchTrips(store)) {
        return false;
    }
    rebuildIndexes(store);
    return !requiresRouteMigration || saveTrips(store);
}

bool studentHasTripHold(const Store& store, const string& studentId, int tripId) {
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        const Reservation& reservation = node->value;
        if (reservation.studentId == studentId && reservation.tripId == tripId
            && occupiesSeat(reservation)) {
            return true;
        }
    }
    bool waiting = false;
    store.waitlist.forEach([&](const WaitlistEntry& entry) {
        if (entry.studentId == studentId && entry.tripId == tripId) {
            waiting = true;
        }
    });
    return waiting;
}

bool enqueueWaitlist(Store& store, const WaitlistEntry& entry) {
    const Queue<WaitlistEntry> previous = store.waitlist;
    store.waitlist.enqueue(entry);
    if (!saveWaitlist(store)) {
        store.waitlist = previous;
        return false;
    }
    return true;
}

bool submitReservationRequest(Store& store, const ReservationRequest& request,
                              string& outcome) {
    store.reservationQueue.enqueue(request);
    bool accepted = false;
    while (!store.reservationQueue.empty()) {
        const ReservationRequest current = store.reservationQueue.front();
        store.reservationQueue.dequeue();

        Trip* trip = findTrip(store, current.tripId);
        if (trip == nullptr || !isOpenTrip(*trip) || tripAtCutoff(*trip)) {
            outcome = "That trip is not available for reservation.";
            continue;
        }
        if (studentHasTripHold(store, current.studentId, current.tripId)) {
            outcome = "You already have a reservation or waitlist hold for that trip.";
            continue;
        }
        if (isOnlinePayment(current.paymentMethod)
            && !current.referenceNumber.empty()
            && store.paymentRefs.contains(current.referenceNumber)) {
            outcome = "That reference number is already logged.";
            continue;
        }

        if (activePassengerCount(store, current.tripId) >= trip->capacity) {
            WaitlistEntry entry;
            entry.tripId = current.tripId;
            entry.studentId = current.studentId;
            entry.paymentMethod = current.paymentMethod;
            entry.referenceNumber = current.referenceNumber;
            if (!enqueueWaitlist(store, entry)) {
                outcome = "Waitlist request could not be saved.";
                continue;
            }
            upsertProfile(store, current.studentId, current.paymentMethod);
            saveProfiles(store);
            outcome = "WAITLIST";
            accepted = true;
            continue;
        }

        Reservation reservation;
        reservation.studentId = current.studentId;
        reservation.date = trip->date;
        reservation.tripId = trip->id;
        reservation.shuttleNumber = trip->shuttleNumber;
        reservation.scheduleNumber = trip->scheduleNumber;
        reservation.paymentMethod = current.paymentMethod;
        reservation.paymentStatus = "Pending";
        reservation.status = "Reserved";
        reservation.referenceNumber = current.referenceNumber;
        reservation.seatNumber = nextSeatNumber(store, current.tripId, trip->capacity);
        reservation.ticketCode = makeTicketCode(reservation);
        Reservation* saved = appendReservationRecord(store, reservation);
        if (saved == nullptr) {
            outcome = "Reservation could not be created.";
            continue;
        }
        upsertProfile(store, current.studentId, current.paymentMethod);
        if (!saveReservations(store) || !saveProfiles(store)) {
            outcome = "Reservation was not created because it could not be saved.";
            continue;
        }
        rebuildIndexes(store);
        outcome = "RESERVED";
        accepted = true;
    }
    return accepted;
}

bool promoteWaitlistForTrip(Store& store, int tripId) {
    Trip* trip = findTrip(store, tripId);
    if (trip == nullptr || !isOpenTrip(*trip)) {
        return false;
    }
    bool promoted = false;
    while (activePassengerCount(store, tripId) < trip->capacity) {
        Queue<WaitlistEntry> remaining;
        WaitlistEntry chosen;
        bool found = false;
        while (!store.waitlist.empty()) {
            WaitlistEntry entry = store.waitlist.front();
            store.waitlist.dequeue();
            if (!found && entry.tripId == tripId) {
                chosen = entry;
                found = true;
            } else {
                remaining.enqueue(entry);
            }
        }
        store.waitlist = remaining;
        if (!found) {
            break;
        }
        if (studentHasTripHold(store, chosen.studentId, tripId)) {
            continue;
        }
        if (isOnlinePayment(chosen.paymentMethod)
            && store.paymentRefs.contains(chosen.referenceNumber)) {
            continue;
        }

        Reservation reservation;
        reservation.studentId = chosen.studentId;
        reservation.date = trip->date;
        reservation.tripId = trip->id;
        reservation.shuttleNumber = trip->shuttleNumber;
        reservation.scheduleNumber = trip->scheduleNumber;
        reservation.paymentMethod = chosen.paymentMethod;
        reservation.paymentStatus = "Pending";
        reservation.status = "Reserved";
        reservation.referenceNumber = chosen.referenceNumber;
        reservation.seatNumber = nextSeatNumber(store, tripId, trip->capacity);
        reservation.ticketCode = makeTicketCode(reservation);
        Reservation* saved = appendReservationRecord(store, reservation);
        if (saved != nullptr && !saved->referenceNumber.empty()) {
            store.paymentRefs.put(saved->referenceNumber, saved);
        }
        upsertProfile(store, chosen.studentId, chosen.paymentMethod);
        promoted = true;
    }
    if (promoted) {
        saveReservations(store);
        saveWaitlist(store);
        saveProfiles(store);
        rebuildIndexes(store);
    } else {
        saveWaitlist(store);
    }
    return promoted;
}

int applyCutoffAndReallocate(Store& store) {
    int released = 0;
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        Reservation& reservation = node->value;
        if (!occupiesSeat(reservation) || reservation.tripId == 0) {
            continue;
        }
        const Trip* trip = findTrip(store, reservation.tripId);
        if (trip == nullptr || !isOpenTrip(*trip) || !tripAtCutoff(*trip)) {
            continue;
        }
        if (reservation.status == "Boarded") {
            continue;
        }
        if (reservation.paymentStatus == "Pending") {
            reservation.status = "Expired";
        } else {
            reservation.status = "No-show";
        }
        ++released;
    }
    if (released > 0) {
        saveReservations(store);
        rebuildIndexes(store);
        for (int i = 0; i < store.trips.size(); ++i) {
            if (isOpenTrip(store.trips[i])) {
                promoteWaitlistForTrip(store, store.trips[i].id);
            }
        }
    }
    return released;
}

void collectTripPassengers(Store& store, int tripId, bool activeOnly,
                           Array<Reservation*>& passengers) {
    passengers.clear();
    for (auto* node = store.reservations.head(); node != nullptr; node = node->next) {
        if (node->value.tripId == tripId
            && (!activeOnly || occupiesSeat(node->value))) {
            passengers.push(&node->value);
        }
    }
}

void collectAssignedTrips(Store& store, const string& driver, bool openOnly,
                          Array<Trip*>& assigned) {
    assigned.clear();
    for (int i = 0; i < store.trips.size(); ++i) {
        if (store.trips[i].driver == driver
            && (!openOnly || isOpenTrip(store.trips[i]))) {
            assigned.push(&store.trips[i]);
        }
    }
}

void printDigitalPass(const Reservation& reservation, const Store& store) {
    cout << "\n========== DIGITAL PASS ==========\n";
    if (reservation.tripId == 0) {
        cout << "Legacy reservation without a dated trip.\n";
    } else {
        const Trip* trip = findTrip(store, reservation.tripId);
        if (trip != nullptr) {
            cout << tripDescription(*trip) << '\n';
        }
    }
    cout << "Student ID: " << reservation.studentId << '\n'
         << "Seat: " << (reservation.seatNumber > 0
                             ? to_string(reservation.seatNumber)
                             : string("Unassigned"))
         << '\n'
         << "Ticket: " << reservation.ticketCode << '\n'
         << "Payment: " << reservation.paymentMethod
         << " | Status: " << reservation.paymentStatus << '\n';
    if (isOnlinePayment(reservation.paymentMethod)) {
        cout << "Reference: "
             << (reservation.referenceNumber.empty() ? "(none)"
                                                     : reservation.referenceNumber)
             << '\n';
    }
    cout << "Reservation: " << reservation.status << '\n'
         << "Verification payload:\n"
         << verificationPayload(reservation) << '\n'
         << "Present this pass at boarding.\n"
         << "=================================\n";
}
