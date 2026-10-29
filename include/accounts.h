#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include "ds/hashtable.h"

#include <string>

struct User {
    std::string identifier;
    std::string password;
};

struct AccountBook {
    HashTable<User> students;
    HashTable<User> drivers;
};

bool loadAccounts(AccountBook& accounts);
bool registerUser(AccountBook& accounts);
bool login(AccountBook& accounts);

#endif
