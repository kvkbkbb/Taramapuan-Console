#include "accounts.h"
#include "dashboards.h"
#include "io_util.h"
#include "paths.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

namespace {

HashTable<User>& tableFor(AccountBook& accounts, bool isStudent) {
    return isStudent ? accounts.students : accounts.drivers;
}

const HashTable<User>& tableFor(const AccountBook& accounts, bool isStudent) {
    return isStudent ? accounts.students : accounts.drivers;
}

bool saveAccount(const User& user, bool isStudent) {
    ofstream file(paths::accounts(), ios::app);
    if (!file.is_open()) {
        cerr << "Could not open " << paths::accounts() << " for saving.\n";
        return false;
    }

    file << (isStudent ? 'S' : 'D') << ' '
         << quoted(user.identifier) << ' ' << quoted(user.password) << '\n';
    file.flush();
    if (!file) {
        cerr << "Could not save account to " << paths::accounts() << ".\n";
        return false;
    }
    return true;
}

}

bool loadAccounts(AccountBook& accounts) {
    ifstream file;
    if (!openOptionalInput(paths::accounts(), file)) {
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
        char type;
        User user;
        if (!(record >> type >> quoted(user.identifier) >> quoted(user.password))) {
            cerr << "Invalid account record in " << paths::accounts() << " at line "
                 << lineNumber << ".\n";
            return false;
        }
        record >> ws;
        if (!record.eof() || user.identifier.empty() || user.password.empty()) {
            cerr << "Invalid account record in " << paths::accounts() << " at line "
                 << lineNumber << ".\n";
            return false;
        }

        bool isStudent = false;
        if (type == 'S') {
            isStudent = true;
        } else if (type == 'D') {
            isStudent = false;
        } else {
            cerr << "Unknown account type in " << paths::accounts() << " at line "
                 << lineNumber << ".\n";
            return false;
        }

        HashTable<User>& table = tableFor(accounts, isStudent);
        if (table.contains(user.identifier)) {
            cerr << "Duplicate account in " << paths::accounts() << " at line "
                 << lineNumber << ".\n";
            return false;
        }
        table.put(user.identifier, user);
    }

    if (file.bad()) {
        cerr << "Error while reading " << paths::accounts() << ".\n";
        return false;
    }

    cout << "Accounts loaded from " << paths::accounts()
         << " into a hash table (" << accounts.students.size()
         << " students, " << accounts.drivers.size() << " drivers).\n";
    return true;
}

bool registerUser(AccountBook& accounts) {
    string identifier;
    string password;
    string typeChoice;

    cout << "Register as:\n1. Student\n2. Driver\nChoose an option: ";
    getline(cin, typeChoice);
    if (typeChoice != "1" && typeChoice != "2") {
        cout << "Invalid user type. Choose 1 for student or 2 for driver.\n";
        return false;
    }

    const bool isStudent = typeChoice == "1";
    cout << (isStudent ? "Enter your student number/ID: " : "Choose a username: ");
    getline(cin, identifier);
    cout << "Choose a password: ";
    getline(cin, password);

    if (identifier.empty() || password.empty()) {
        cout << (isStudent ? "Student number/ID and password cannot be empty.\n"
                           : "Username and password cannot be empty.\n");
        return false;
    }

    HashTable<User>& table = tableFor(accounts, isStudent);
    if (table.contains(identifier)) {
        cout << (isStudent ? "That student number/ID is already registered.\n"
                           : "That username is already registered.\n");
        return false;
    }

    const User user{identifier, password};
    if (!saveAccount(user, isStudent)) {
        cout << "Account was not registered because it could not be saved.\n";
        return false;
    }

    table.put(identifier, user);
    cout << (isStudent ? "student" : "driver") << " account created successfully.\n";
    return true;
}

bool login(AccountBook& accounts) {
    string identifier;
    string password;
    string typeChoice;

    cout << "Log in as:\n1. Student\n2. Driver\nChoose an option: ";
    getline(cin, typeChoice);
    if (typeChoice != "1" && typeChoice != "2") {
        cout << "Invalid user type. Choose 1 for student or 2 for driver.\n";
        return false;
    }

    const bool isStudent = typeChoice == "1";
    cout << (isStudent ? "Student number/ID: " : "Username: ");
    getline(cin, identifier);
    cout << "Password: ";
    getline(cin, password);

    const User* user = tableFor(accounts, isStudent).get(identifier);
    if (user == nullptr || user->password != password) {
        cout << (isStudent ? "Invalid student number/ID or password.\n"
                           : "Invalid username or password.\n");
        return false;
    }

    cout << "Welcome, " << (isStudent ? "student " : "driver ") << identifier << "!\n";
    if (isStudent) {
        studentDashboard(identifier);
    } else {
        driverDashboard(identifier);
    }
    return true;
}
