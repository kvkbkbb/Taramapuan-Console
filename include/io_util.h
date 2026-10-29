#ifndef IO_UTIL_H
#define IO_UTIL_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

bool openOptionalInput(const std::string& fileName, std::ifstream& file);

template <typename Writer>
bool replaceFile(const std::string& fileName, Writer writer) {
    const std::string temporaryName = fileName + ".tmp";
    const std::string backupName = fileName + ".bak";
    {
        std::ofstream temporary(temporaryName, std::ios::trunc);
        if (!temporary.is_open()) {
            std::cerr << "Could not open " << temporaryName << " for saving.\n";
            return false;
        }
        writer(temporary);
        temporary.flush();
        if (!temporary) {
            std::cerr << "Could not write " << temporaryName << ".\n";
            return false;
        }
    }

    std::error_code error;
    const bool originalExists = std::filesystem::exists(fileName, error);
    if (error) {
        std::cerr << "Could not check for " << fileName << ": " << error.message() << '\n';
        return false;
    }
    if (originalExists) {
        std::filesystem::rename(fileName, backupName, error);
        if (error) {
            std::cerr << "Could not preserve " << fileName << ": " << error.message() << '\n';
            return false;
        }
    }

    std::filesystem::rename(temporaryName, fileName, error);
    if (error) {
        std::cerr << "Could not replace " << fileName << ": " << error.message() << '\n';
        if (originalExists) {
            std::error_code restoreError;
            std::filesystem::rename(backupName, fileName, restoreError);
            if (restoreError) {
                std::cerr << "Could not restore " << fileName << ": "
                          << restoreError.message() << '\n';
            }
        }
        return false;
    }

    if (originalExists) {
        std::filesystem::remove(backupName, error);
        if (error) {
            std::cerr << "Could not remove backup " << backupName << ": "
                      << error.message() << '\n';
        }
    }
    return true;
}

#endif
