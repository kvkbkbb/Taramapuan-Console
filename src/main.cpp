#include "accounts.h"
#include "paths.h"

#include <iostream>
#include <string>

using namespace std;

int main() {
    AccountBook accounts;
    string choice;

    cout << "========================================\n"
         << "           TaraMapuan Console\n"
         << " Campus shuttle reservation & monitoring\n"
         << "========================================\n";

    if (!paths::initialize()) {
        return 1;
    }
    cout << "Data folder: " << paths::dataDir() << '\n';

    if (!loadAccounts(accounts)) {
        return 1;
    }

    while (true) {
        cout << "\n1. Register\n2. Log in\n3. Exit\nChoose an option: ";
        if (!getline(cin, choice)) {
            break;
        }

        if (choice == "1") {
            registerUser(accounts);
        } else if (choice == "2") {
            login(accounts);
        } else if (choice == "3") {
            break;
        } else {
            cout << "Invalid option. Choose 1, 2, or 3.\n";
        }
    }

    return 0;
}
