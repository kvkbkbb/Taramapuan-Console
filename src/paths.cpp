#include "paths.h"

#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

std::string g_dataDir;

std::filesystem::path executableDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buffer).parent_path();
#else
    std::error_code error;
    std::filesystem::path cwd = std::filesystem::current_path(error);
    return error ? std::filesystem::path(".") : cwd;
#endif
}

bool hasDataFiles(const std::filesystem::path& dir) {
    std::error_code error;
    return std::filesystem::exists(dir / "accounts.txt", error)
        || std::filesystem::exists(dir / "driver_trips.txt", error)
        || std::filesystem::exists(dir / "reservations.txt", error);
}

bool tryUse(const std::filesystem::path& dir, bool createIfMissing) {
    std::error_code error;
    if (std::filesystem::exists(dir, error) && std::filesystem::is_directory(dir, error)) {
        g_dataDir = dir.lexically_normal().string();
        return true;
    }
    if (!createIfMissing) {
        return false;
    }
    if (std::filesystem::create_directories(dir, error) && !error) {
        g_dataDir = dir.lexically_normal().string();
        return true;
    }
    return false;
}

std::string joinFile(const char* fileName) {
    return (std::filesystem::path(g_dataDir) / fileName).string();
}

}

namespace paths {

bool initialize() {
    if (!g_dataDir.empty()) {
        return true;
    }

    const std::filesystem::path exeDir = executableDirectory();
    std::error_code error;
    const std::filesystem::path cwd = std::filesystem::current_path(error);

    const std::filesystem::path candidates[] = {
        cwd / "data",
        exeDir / "data",
        exeDir.parent_path() / "data"
    };

    for (const std::filesystem::path& candidate : candidates) {
        if (hasDataFiles(candidate) && tryUse(candidate, false)) {
            return true;
        }
    }
    for (const std::filesystem::path& candidate : candidates) {
        if (tryUse(candidate, false)) {
            return true;
        }
    }
    for (const std::filesystem::path& candidate : candidates) {
        if (tryUse(candidate, true)) {
            return true;
        }
    }

    std::cerr << "Could not locate or create the data folder.\n";
    return false;
}

const std::string& dataDir() {
    initialize();
    return g_dataDir;
}

std::string accounts() {
    return joinFile("accounts.txt");
}

std::string trips() {
    return joinFile("driver_trips.txt");
}

std::string reservations() {
    return joinFile("reservations.txt");
}

std::string studentProfiles() {
    return joinFile("student_profiles.txt");
}

std::string waitlist() {
    return joinFile("waitlist.txt");
}

}
