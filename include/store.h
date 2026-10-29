#ifndef STORE_H
#define STORE_H

#include "ds/array.h"
#include "ds/hashtable.h"
#include "ds/linkedlist.h"
#include "ds/queue.h"
#include "models.h"

#include <string>

const int kCutoffMinutes = 5;

struct Store {
    Array<Trip> trips;
    LinkedList<Reservation> reservations;
    Queue<ReservationRequest> reservationQueue;
    Queue<WaitlistEntry> waitlist;
    Array<StudentProfile> profiles;
    HashTable<Reservation*> paymentRefs;
    HashTable<Reservation*> tickets;
    HashTable<StudentProfile> studentDirectory;
};

bool isValidDate(const std::string& date);
std::string today();
std::string nowTime();
int clockToMinutes(const std::string& clock);
int currentMinutes();
bool tripAtCutoff(const Trip& trip);
bool tripHasDeparted(const Trip& trip);
bool isValidPayment(const std::string& method);
bool isOnlinePayment(const std::string& method);
bool isActiveReservation(const Reservation& reservation);
bool isOpenTrip(const Trip& trip);
bool occupiesSeat(const Reservation& reservation);

const Trip* findTrip(const Store& store, int tripId);
Trip* findTrip(Store& store, int tripId);
int activePassengerCount(const Store& store, int tripId);
int nextSeatNumber(const Store& store, int tripId, int capacity);
std::string tripDescription(const Trip& trip);
std::string makeTicketCode(const Reservation& reservation);
std::string verificationPayload(const Reservation& reservation);

bool loadStore(Store& store);
bool saveTrips(const Store& store);
bool saveReservations(const Store& store);
bool saveWaitlist(const Store& store);
bool saveProfiles(const Store& store);
bool appendTrip(Store& store, const Trip& trip);

void rebuildIndexes(Store& store);
int applyCutoffAndReallocate(Store& store);
bool promoteWaitlistForTrip(Store& store, int tripId);
bool enqueueWaitlist(Store& store, const WaitlistEntry& entry);
bool studentHasTripHold(const Store& store, const std::string& studentId, int tripId);
bool submitReservationRequest(Store& store, const ReservationRequest& request,
                              std::string& outcome);

void collectTripPassengers(Store& store, int tripId, bool activeOnly,
                           Array<Reservation*>& passengers);
void collectAssignedTrips(Store& store, const std::string& driver, bool openOnly,
                          Array<Trip*>& assigned);

void printDigitalPass(const Reservation& reservation, const Store& store);
void upsertProfile(Store& store, const std::string& studentId,
                   const std::string& paymentMethod);

#endif
