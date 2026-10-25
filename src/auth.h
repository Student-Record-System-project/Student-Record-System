#ifndef AUTH_H
#define AUTH_H

#include <string>
#include "sqlite3.h"

bool initializeAuth(sqlite3* db);
bool setupFirstAdmin(sqlite3* db);
bool loginUser(sqlite3* db, std::string& username, std::string& role);
bool createUser(sqlite3* db, const std::string& username,
                const std::string& password, const std::string& role);

#endif
