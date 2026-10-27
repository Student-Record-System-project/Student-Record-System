#include "auth.h"
#include "password_hash.h"

#include <iostream>
#include <cctype>
#include <ctime>
#include <limits>

using namespace std;

static bool validUsername(const string& username) {
    if (username.size() < 3 || username.size() > 50) return false;
    for (unsigned char c : username) {
        if (!isalnum(c) && c != '_' && c != '-') return false;
    }
    return true;
}

static bool validRole(const string& role) {
    return role == "Admin" || role == "Faculty" ||
           role == "Staff" || role == "Student";
}

static bool execute(sqlite3* db, const char* sql) {
    char* error = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        cerr << "Authentication database error: "
             << (error ? error : sqlite3_errmsg(db)) << '\n';
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool initializeAuth(sqlite3* db) {
    return execute(db,
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "password_hash TEXT NOT NULL,"
        "role TEXT NOT NULL CHECK(role IN "
        "('Admin','Faculty','Staff','Student')),"
        "failed_attempts INTEGER NOT NULL DEFAULT 0,"
        "locked_until INTEGER NOT NULL DEFAULT 0"
        ");");
}

bool createUser(sqlite3* db, const string& username,
                const string& password, const string& role) {
    if (!validUsername(username)) {
        cout << "Username must be 3-50 letters, digits, _ or -.\n";
        return false;
    }
    if (password.size() < 10 || password.size() > 128) {
        cout << "Password must contain 10-128 characters.\n";
        return false;
    }
    if (!validRole(role)) {
        cout << "Invalid role.\n";
        return false;
    }

    string encoded;
    if (!hashPassword(password, encoded)) {
        cout << "Password hashing failed.\n";
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO users(username,password_hash,role) VALUES(?,?,?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Could not prepare account creation.\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, encoded.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cout << "Account creation failed (username may already exist).\n";
        return false;
    }

    cout << "Account created successfully.\n";
    return true;
}

bool setupFirstAdmin(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Could not check administrator setup.\n";
        return false;
    }

    int rc = sqlite3_step(stmt);
    int count = (rc == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : -1;
    sqlite3_finalize(stmt);

    if (count < 0) return false;
    if (count > 0) return true;

    cout << "\n=== First-Time Administrator Setup ===\n";
    string username, password;
    cout << "Create admin username: ";
    getline(cin, username);
    cout << "Create admin password (input is visible): ";
    getline(cin, password);

    if (!createUser(db, username, password, "Admin")) {
        cout << "Administrator setup was not completed.\n";
        return false;
    }
    return true;
}

bool loginUser(sqlite3* db, string& username, string& role) {
    string password;
    cout << "Username: ";
    getline(cin, username);
    cout << "Password (input is visible): ";
    getline(cin, password);

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT password_hash, role, failed_attempts, locked_until "
        "FROM users WHERE username=?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Login database error.\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        cout << "Invalid username or password.\n";
        return false;
    }

    const unsigned char* hashText = sqlite3_column_text(stmt, 0);
    const unsigned char* roleText = sqlite3_column_text(stmt, 1);
    string encoded = hashText ? reinterpret_cast<const char*>(hashText) : "";
    string storedRole = roleText ? reinterpret_cast<const char*>(roleText) : "";
    int failures = sqlite3_column_int(stmt, 2);
    sqlite3_int64 lockedUntil = sqlite3_column_int64(stmt, 3);
    sqlite3_finalize(stmt);

    sqlite3_int64 now = static_cast<sqlite3_int64>(time(nullptr));
    if (lockedUntil > now) {
        cout << "Account temporarily locked. Try again in "
             << (lockedUntil - now + 59) / 60 << " minute(s).\n";
        return false;
    }

    if (!verifyPassword(password, encoded)) {
        ++failures;
        sqlite3_stmt* update = nullptr;

        if (failures >= 5) {
            const char* updateSql =
                "UPDATE users SET failed_attempts=0, locked_until=? "
                "WHERE username=?;";
            if (sqlite3_prepare_v2(db, updateSql, -1, &update, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(update, 1, now + 900);
                sqlite3_bind_text(update, 2, username.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(update);
                sqlite3_finalize(update);
            }
            cout << "Five failed attempts. Account locked for 15 minutes.\n";
        } else {
            const char* updateSql =
                "UPDATE users SET failed_attempts=? WHERE username=?;";
            if (sqlite3_prepare_v2(db, updateSql, -1, &update, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(update, 1, failures);
                sqlite3_bind_text(update, 2, username.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(update);
                sqlite3_finalize(update);
            }
            cout << "Invalid username or password. Attempts remaining: "
                 << (5 - failures) << '\n';
        }
        return false;
    }

    sqlite3_stmt* reset = nullptr;
    if (sqlite3_prepare_v2(db,
            "UPDATE users SET failed_attempts=0, locked_until=0 WHERE username=?;",
            -1, &reset, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(reset, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(reset);
        sqlite3_finalize(reset);
    }

    role = storedRole;
    cout << "Login successful. Role: " << role << '\n';
    return true;
}
