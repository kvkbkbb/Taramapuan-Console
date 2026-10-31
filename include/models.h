#ifndef MODELS_H
#define MODELS_H

#include "ds/array.h"

#include <string>

struct Schedule {
    std::string direction;
    std::string departureTime;
};

struct Shuttle {
    int number = 0;
    std::string name;
    std::string route;
    Array<Schedule> schedules;
    Array<int> stopIds;
};

struct Trip {
    int id = 0;
    std::string driver;
    std::string date;
    int shuttleNumber = 0;
    int scheduleNumber = 0;
    int capacity = 0;
    std::string status;
    std::string routeStops;
    double distanceKm = 0.0;
};

struct Reservation {
    std::string studentId;
    std::string date;
    int tripId = 0;
    int shuttleNumber = 0;
    int scheduleNumber = 0;
    std::string paymentMethod;
    std::string paymentStatus;
    std::string status;
    std::string referenceNumber;
    std::string ticketCode;
    int seatNumber = 0;
    int stopIndex = -1;
    std::string routeStops;
    double distanceKm = 0.0;
};

struct WaitlistEntry {
    int tripId = 0;
    std::string studentId;
    std::string paymentMethod;
    std::string referenceNumber;
    int stopIndex = -1;
};

struct ReservationRequest {
    std::string studentId;
    int tripId = 0;
    std::string paymentMethod;
    std::string referenceNumber;
    int stopIndex = -1;
};

struct StudentProfile {
    std::string studentId;
    std::string paymentMethod;
};

#endif
