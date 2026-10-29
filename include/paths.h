#ifndef PATHS_H
#define PATHS_H

#include <string>

namespace paths {

bool initialize();
const std::string& dataDir();
std::string accounts();
std::string trips();
std::string reservations();
std::string studentProfiles();
std::string waitlist();

}

#endif
