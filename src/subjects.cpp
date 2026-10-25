#include "subjects.h"

#include <iostream>
#include <string>

using namespace std;

bool initializeSubjects(sqlite3* db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS subjects ("
        "subject_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "subject_code TEXT NOT NULL UNIQUE,"
        "subject_name TEXT NOT NULL"
        ");";

    char* error = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        cerr << "Subject table setup failed: "
             << (error ? error : sqlite3_errmsg(db)) << '\n';
        sqlite3_free(error);
        return false;
    }
    return true;
}

void addSubject(sqlite3* db) {
    string code, name;

    cout << "Subject code: ";
    getline(cin, code);
    cout << "Subject name: ";
    getline(cin, name);

    if (code.empty() || name.empty()) {
        cout << "Subject code and name are required.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO subjects(subject_code, subject_name) VALUES(?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        cout << "Subject added successfully. ID: "
             << sqlite3_last_insert_rowid(db) << '\n';
    } else {
        cout << "Could not add subject (subject code may already exist): "
             << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void listSubjects(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT subject_id, subject_code, subject_name "
        "FROM subjects ORDER BY subject_code;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    cout << "\n--- Subjects ---\n";
    bool found = false;
    int rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        found = true;
        cout << "ID: " << sqlite3_column_int(stmt, 0)
             << " | Code: "
             << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))
             << " | Name: "
             << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
             << '\n';
    }

    if (!found && rc == SQLITE_DONE) cout << "No subjects found.\n";
    if (rc != SQLITE_DONE) cout << "Subject listing failed.\n";

    sqlite3_finalize(stmt);
}
