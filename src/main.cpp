
#include <iostream>
#include <string>
#include <limits>
#include "sqlite3.h"
#include <cctype>

using namespace std;

std::string role = "admin";   // replace with acutal when auth is implemeted

bool createStudent(sqlite3* db) {
    string id, name, email;

    cout << "\nStudent ID: ";
    getline(cin, id);
    cout << "Name: ";
    getline(cin, name);
    cout << "Email: ";
    getline(cin, email);

    if (id.empty() || name.empty()) {
        cout << "Error: Student ID and name are required.\n";
        return false;
    }

    const char* sql =
        "INSERT INTO students (student_id, name, email) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, email.c_str(), -1, SQLITE_TRANSIENT);

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

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        auto textColumn = [&](int column) -> const char* {
            const unsigned char* value = sqlite3_column_text(stmt, column);
            return value ? reinterpret_cast<const char*>(value) : "";
        };

        cout << "ID: " << textColumn(0)
             << " | Name: " << textColumn(1)
             << " | Email: " << textColumn(2) << '\n';
    }

    if (!found) {
        cout << "No student records found.\n";
    }

    sqlite3_finalize(stmt);
}

void searchStudent(sqlite3* db) {
    string id;
    cout << "\nEnter student ID to search: ";
    getline(cin, id);

    const char* sql =
        "SELECT student_id, name, email FROM students WHERE student_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto textColumn = [&](int column) -> const char* {
            const unsigned char* value = sqlite3_column_text(stmt, column);
            return value ? reinterpret_cast<const char*>(value) : "";
        };

        cout << "\nStudent found:\n"
             << "ID: " << textColumn(0) << '\n'
             << "Name: " << textColumn(1) << '\n'
             << "Email: " << textColumn(2) << '\n';
    } else {
        cout << "No student found with ID: " << id << '\n';
    }

    sqlite3_finalize(stmt);
}


static std::string trimCopy(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// role: "admin" or "teacher" are allowed; anything else is rejected.(this is fask case)
bool updateStudent(sqlite3* db, const std::string& role) {
    // ---- 1. Permission ----
    if (role != "admin" && role != "teacher") {
        std::cout << "Access denied: only admins and teachers can update records.\n";
        return false;
    }

    // ---- 2. Pick record ----
    std::cout << "\nEnter student ID to update: ";
    std::string oldId;
    std::getline(std::cin, oldId);
    oldId = trimCopy(oldId);

    if (oldId.empty()) {
        std::cout << "Error: student ID is required.\n";
        return false;
    }

    // ---- 3. Load current values ----
    sqlite3_stmt* fetch = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT name, email FROM students WHERE student_id = ?;",
            -1, &fetch, nullptr) != SQLITE_OK) {
        std::cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }
    sqlite3_bind_text(fetch, 1, oldId.c_str(), -1, SQLITE_TRANSIENT);

    std::string curName, curEmail;
    if (sqlite3_step(fetch) == SQLITE_ROW) {
        curName  = reinterpret_cast<const char*>(sqlite3_column_text(fetch, 0));
        const unsigned char* em = sqlite3_column_text(fetch, 1);
        curEmail = em ? reinterpret_cast<const char*>(em) : "";
    } else {
        sqlite3_finalize(fetch);
        std::cout << "Error: no student found with ID '" << oldId << "'.\n";
        return false;
    }
    sqlite3_finalize(fetch);

    // ---- 4. Ask for new values (Enter = keep current) ----
    std::cout << "\nCurrent record:\n"
              << "  Name : " << curName  << "\n"
              << "  Email: " << curEmail << "\n"
              << "Press Enter to keep a value unchanged.\n";

    std::cout << "New name [" << curName << "]: ";
    std::string newName;
    std::getline(std::cin, newName);
    newName = trimCopy(newName);
    if (newName.empty()) newName = curName;

    std::cout << "New email[" << curEmail << "]: ";
    std::string newEmail;
    std::getline(std::cin, newEmail);
    newEmail = trimCopy(newEmail);
    if (newEmail.empty()) newEmail = curEmail;

    // ---- 5. Validate ----
    if (newName.empty()) {
        std::cout << "Error: name is required. Update rejected.\n";
        return false;
    }
    if (!newEmail.empty()) {
        auto at  = newEmail.find('@');
        auto dot = newEmail.find('.', at == std::string::npos ? 0 : at);
        if (at == std::string::npos || at == 0 ||
            dot == std::string::npos || dot == newEmail.size() - 1 ||
            newEmail.find(' ') != std::string::npos) {
            std::cout << "Error: email format is invalid. Update rejected.\n";
            return false;
        }
    }

    // ---- 6. Save ----
    sqlite3_stmt* upd = nullptr;
    if (sqlite3_prepare_v2(db,
            "UPDATE students SET name = ?, email = ? WHERE student_id = ?;",
            -1, &upd, nullptr) != SQLITE_OK) {
        std::cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }
    sqlite3_bind_text(upd, 1, newName.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(upd, 2, newEmail.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(upd, 3, oldId.c_str(),    -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(upd);
    sqlite3_finalize(upd);

    if (rc != SQLITE_DONE) {
        std::cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    std::cout << "\nStudent record updated successfully.\n"
              << "  ID   : " << oldId    << "\n"
              << "  Name : " << newName  << "\n"
              << "  Email: " << newEmail << "\n";
    return true;
}


int main() {
    sqlite3* db = nullptr;

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

    int choice;
    do {
        cout << "\n=== Student Record System ===\n"
             << "1. Create student record\n"
             << "2. View all students\n"
             << "3. Search student by ID\n"
             << "4. Update record \n"
             << "0. Exit\n"
             << "Choose: ";

        if (!(cin >> choice)) {
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
            case 4: updateStudent(db, role); break;
            case 0: cout << "Application closed.\n"; break;
            default: cout << "Invalid option.\n";
        }
    } while (choice != 0);

    sqlite3_close(db);
    return 0;
}
