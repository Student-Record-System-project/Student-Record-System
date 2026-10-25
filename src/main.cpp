#include <iostream>
#include <string>
#include <limits>
#include <cctype>
#include "sqlite3.h"

using namespace std;

std::string role = "admin";   // replace with actual when auth is implemented

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
    if (email.empty()) return true;
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

// ============================================================
// Update student record — admin/teacher only
// ============================================================
bool updateStudent(sqlite3* db, const std::string& role) {
    if (role != "admin" && role != "teacher") {
        std::cout << "Access denied: only admins and teachers can update records.\n";
        return false;
    }

    std::cout << "\nEnter student ID to update: ";
    std::string oldId;
    std::getline(std::cin, oldId);
    oldId = trim(oldId);

    if (oldId.empty()) {
        std::cout << "Error: student ID is required.\n";
        return false;
    }

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

    std::cout << "\nCurrent record:\n"
              << "  Name : " << curName  << "\n"
              << "  Email: " << curEmail << "\n"
              << "Press Enter to keep a value unchanged.\n";

    std::cout << "New name [" << curName << "]: ";
    std::string newName;
    std::getline(std::cin, newName);
    newName = trim(newName);
    if (newName.empty()) newName = curName;

    std::cout << "New email[" << curEmail << "]: ";
    std::string newEmail;
    std::getline(std::cin, newEmail);
    newEmail = trim(newEmail);
    if (newEmail.empty()) newEmail = curEmail;

    if (!validName(newName)) {
        std::cout << "Error: name is required and must be 1-100 characters. Update rejected.\n";
        return false;
    }
    if (!validEmail(newEmail)) {
        std::cout << "Error: email format is invalid. Update rejected.\n";
        return false;
    }

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

// ============================================================
// Delete student record — admin/teacher only
// ============================================================
bool deleteStudent(sqlite3* db, const std::string& role) {
    if (role != "admin" && role != "teacher") {
        std::cout << "Access denied: only admins and teachers can delete records.\n";
        return false;
    }

    std::cout << "\nEnter student ID to delete: ";
    std::string id;
    std::getline(std::cin, id);
    id = trim(id);

    if (id.empty()) {
        std::cout << "Error: student ID is required.\n";
        return false;
    }

    sqlite3_stmt* fetch = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT name, email FROM students WHERE student_id = ?;",
            -1, &fetch, nullptr) != SQLITE_OK) {
        std::cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }
    sqlite3_bind_text(fetch, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    std::string curName, curEmail;
    if (sqlite3_step(fetch) == SQLITE_ROW) {
        curName  = reinterpret_cast<const char*>(sqlite3_column_text(fetch, 0));
        const unsigned char* em = sqlite3_column_text(fetch, 1);
        curEmail = em ? reinterpret_cast<const char*>(em) : "";
    } else {
        sqlite3_finalize(fetch);
        std::cout << "Error: no student found with ID '" << id << "'.\n";
        return false;
    }
    sqlite3_finalize(fetch);

    std::cout << "\nAbout to delete this record:\n"
              << "  ID   : " << id       << "\n"
              << "  Name : " << curName  << "\n"
              << "  Email: " << curEmail << "\n"
              << "\nRelated grades and attendance will also be removed.\n"
              << "Type 'yes' to confirm, anything else to cancel: ";

    std::string confirm;
    std::getline(std::cin, confirm);
    confirm = trim(confirm);

    if (confirm != "yes") {
        std::cout << "Deletion cancelled. Record was not removed.\n";
        return false;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    sqlite3_stmt* del = nullptr;
    if (sqlite3_prepare_v2(db,
            "DELETE FROM students WHERE student_id = ?;",
            -1, &del, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        std::cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }
    sqlite3_bind_text(del, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(del);
    sqlite3_finalize(del);

    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        std::cout << "Database error while deleting: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    if (sqlite3_changes(db) == 0) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        std::cout << "Error: record was not deleted (it may have been "
                     "removed already).\n";
        return false;
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    std::cout << "\nStudent record deleted successfully.\n"
              << "  Removed ID   : " << id       << "\n"
              << "  Removed Name : " << curName  << "\n"
              << "  Removed Email: " << curEmail << "\n";
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

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

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
             << "4. Update record\n"
             << "5. Delete record\n"
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
            case 4: updateStudent(db, role); break;
            case 5: deleteStudent(db, role); break;
            case 0: cout << "Application closed.\n"; break;
            default: cout << "Invalid option.\n";
        }
    } while (choice != 0);

    sqlite3_close(db);
    return 0;
}
