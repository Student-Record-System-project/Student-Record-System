
#include <iostream>
#include <string>
#include <limits>
#include <cctype>
#include "sqlite3.h"

using namespace std;

// Shared validation helpers. Keep these independent of the SQLite schema.
string trim(const string& value) {
    size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool validStudentId(const string& id) {
    if (id.empty() || id.size() > 50) return false;
    for (unsigned char c : id) {
        if (!isalnum(c) && c != '-' && c != '_') return false;
    }
    return true;
}

bool validName(const string& name) {
    if (name.empty() || name.size() > 100) return false;
    for (unsigned char c : name) {
        if (iscntrl(c)) return false;
    }
    return true;
}

bool validEmail(const string& email) {
    if (email.empty()) return true; // Existing schema permits optional email.
    if (email.size() > 254 || email.find(' ') != string::npos) return false;
    size_t at = email.find('@');
    if (at == string::npos || at == 0 || at != email.rfind('@')) return false;
    size_t dot = email.find('.', at + 2);
    return dot != string::npos && dot > at + 1 && dot + 1 < email.size();
}

bool bindText(sqlite3* db, sqlite3_stmt* stmt, int index, const string& value) {
    int rc = sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) cerr << "Database bind error: " << sqlite3_errmsg(db) << '\n';
    return rc == SQLITE_OK;
}


bool createStudent(sqlite3* db) {
    string id, name, email;

    cout << "\nStudent ID: ";
    getline(cin, id);
    cout << "Name: ";
    getline(cin, name);
    cout << "Email: ";
    getline(cin, email);

    id = trim(id);
    name = trim(name);
    email = trim(email);
    if (!validStudentId(id)) {
        cout << "Error: Student ID must be 1-50 letters, digits, hyphens or underscores.\n";
        return false;
    }
    if (!validName(name)) {
        cout << "Error: Name must be 1-100 characters without control characters.\n";
        return false;
    }
    if (!validEmail(email)) {
        cout << "Error: Enter a valid email address or leave it blank.\n";
        return false;
    }

    const char* sql =
        "INSERT INTO students (student_id, name, email) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    if (!bindText(db, stmt, 1, id) ||
        !bindText(db, stmt, 2, name) ||
        !bindText(db, stmt, 3, email)) {
        sqlite3_finalize(stmt);
        return false;
    }

    int result = sqlite3_step(stmt);

    if (result == SQLITE_DONE) {
        cout << "Student record created successfully.\n";
    } else if (result == SQLITE_CONSTRAINT ||
               result == SQLITE_CONSTRAINT_PRIMARYKEY ||
               result == SQLITE_CONSTRAINT_UNIQUE) {
        cout << "Error: That student ID already exists.\n";
    } else {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    return result == SQLITE_DONE;
}

void viewStudents(sqlite3* db) {
    const char* sql =
        "SELECT student_id, name, email FROM students ORDER BY student_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    cout << "\n--- Student Records ---\n";
    bool found = false;

    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        found = true;
        auto textColumn = [&](int column) -> const char* {
            const unsigned char* value = sqlite3_column_text(stmt, column);
            return value ? reinterpret_cast<const char*>(value) : "";
        };

        cout << "ID: " << textColumn(0)
             << " | Name: " << textColumn(1)
             << " | Email: " << textColumn(2) << '\n';
    }

    if (result != SQLITE_DONE) {
        cout << "Database read error: " << sqlite3_errmsg(db) << '\n';
    } else if (!found) {
        cout << "No student records found.\n";
    }

    sqlite3_finalize(stmt);
}

void searchStudent(sqlite3* db) {
    string id;
    cout << "\nEnter student ID to search: ";
    getline(cin, id);
    id = trim(id);
    if (!validStudentId(id)) {
        cout << "Error: Enter a valid student ID to search.\n";
        return;
    }

    const char* sql =
        "SELECT student_id, name, email FROM students WHERE student_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    if (!bindText(db, stmt, 1, id)) {
        sqlite3_finalize(stmt);
        return;
    }

    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        auto textColumn = [&](int column) -> const char* {
            const unsigned char* value = sqlite3_column_text(stmt, column);
            return value ? reinterpret_cast<const char*>(value) : "";
        };

        cout << "\nStudent found:\n"
             << "ID: " << textColumn(0) << '\n'
             << "Name: " << textColumn(1) << '\n'
             << "Email: " << textColumn(2) << '\n';
    } else if (result == SQLITE_DONE) {
        cout << "No student found with ID: " << id << '\n';
    } else {
        cout << "Database search error: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

int main() {
    sqlite3* db = nullptr;

    // The database directory is not tracked by Git; create data/ before running.
    if (sqlite3_open("data/student_records.db", &db) != SQLITE_OK) {
        cerr << "Cannot open database: "
             << (db ? sqlite3_errmsg(db) : "allocation failed") << '\n';
        if (db) sqlite3_close(db);
        return 1;
    }

    const char* createTable =
        "CREATE TABLE IF NOT EXISTS students ("
        "student_id TEXT PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "email TEXT"
        ");";

    char* error = nullptr;
    if (sqlite3_exec(db, createTable, nullptr, nullptr, &error) != SQLITE_OK) {
        cerr << "Table creation failed: "
             << (error ? error : "unknown error") << '\n';
        sqlite3_free(error);
        sqlite3_close(db);
        return 1;
    }

    int choice = -1;
    do {
        cout << "\n=== Student Record System ===\n"
             << "1. Create student record\n"
             << "2. View all students\n"
             << "3. Search student by ID\n"
             << "0. Exit\n"
             << "Choose: ";

        if (!(cin >> choice)) {
            if (cin.eof()) break;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Please enter a valid number.\n";
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1: createStudent(db); break;
            case 2: viewStudents(db); break;
            case 3: searchStudent(db); break;
            case 0: cout << "Application closed.\n"; break;
            default: cout << "Invalid option.\n";
        }
    } while (choice != 0);

    sqlite3_close(db);
    return 0;
}
