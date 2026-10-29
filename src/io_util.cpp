#include "io_util.h"

#include <filesystem>
#include <iostream>

bool openOptionalInput(const std::string& fileName, std::ifstream& file) {
    file.open(fileName);
    if (file.is_open()) {
        return true;
    }

    std::error_code error;
    const bool fileExists = std::filesystem::exists(fileName, error);
    if (error) {
        std::cerr << "Could not check for " << fileName << ": " << error.message() << '\n';
        return false;
    }
    if (!fileExists) {
        return true;
    }

    std::cerr << "Could not open " << fileName << " for reading.\n";
    return false;
}
