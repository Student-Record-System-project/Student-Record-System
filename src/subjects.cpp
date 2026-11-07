
#include "subjects.h"
#include "audit.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;

static bool columnExists(sqlite3* db, const string& columnName) {
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, "PRAGMA table_info(subjects);",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool exists = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 1);

        if (name && columnName == reinterpret_cast<const char*>(name)) {
            exists = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool initializeSubjects(sqlite3* db) {
    const char* createSql =
        "CREATE TABLE IF NOT EXISTS subjects ("
        "subject_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "subject_code TEXT NOT NULL UNIQUE,"
        "subject_name TEXT NOT NULL"
        ");";

    char* error = nullptr;

    int rc = sqlite3_exec(db, createSql, nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        cerr << "Subject table setup failed: "
             << (error ? error : sqlite3_errmsg(db)) << '\n';
        sqlite3_free(error);
        return false;
    }

    if (!columnExists(db, "credits")) {
        rc = sqlite3_exec(
            db,
            "ALTER TABLE subjects ADD COLUMN credits REAL;",
            nullptr, nullptr, &error
        );

        if (rc != SQLITE_OK) {
            cerr << "Credits migration failed: "
                 << (error ? error : sqlite3_errmsg(db)) << '\n';
            sqlite3_free(error);
            return false;
        }
    }

    if (!columnExists(db, "semester")) {
        rc = sqlite3_exec(
            db,
            "ALTER TABLE subjects ADD COLUMN semester INTEGER;",
            nullptr, nullptr, &error
        );

        if (rc != SQLITE_OK) {
            cerr << "Semester migration failed: "
                 << (error ? error : sqlite3_errmsg(db)) << '\n';
            sqlite3_free(error);
            return false;
        }
    }

    return true;
}

void addSubject(sqlite3* db) {
    string code, name;
    double credits;
    int semester;

    cout << "Subject code: ";
    getline(cin, code);

    cout << "Subject name: ";
    getline(cin, name);

    if (code.empty() || name.empty()) {
        cout << "Subject code and name are required.\n";
        return;
    }

    cout << "Credits: ";

    if (!(cin >> credits) || credits <= 0) {
        cout << "Enter a valid positive credit value.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    cout << "Semester (1-8): ";

    if (!(cin >> semester) || semester < 1 || semester > 8) {
        cout << "Enter a valid semester between 1 and 8.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "INSERT INTO subjects(subject_code, subject_name, credits, semester) "
        "VALUES(?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, credits);
    sqlite3_bind_int(stmt, 4, semester);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_DONE) {
        logAuditEvent(db, "SYSTEM", "SUBJECT_ADDED", std::to_string(static_cast<long long>(sqlite3_last_insert_rowid(db))), "Code: " + code + ", Name: " + name);
        cout << "Subject added successfully. ID: "
             << sqlite3_last_insert_rowid(db) << '\n';
    } else {
        cout << "Could not add subject: "
             << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void listSubjects(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT subject_id, subject_code, subject_name, credits, semester "
        "FROM subjects ORDER BY semester, subject_code;";

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
             << " | Credits: ";

        if (sqlite3_column_type(stmt, 3) == SQLITE_NULL) {
            cout << "Not assigned";
        } else {
            cout << sqlite3_column_double(stmt, 3);
        }

        cout << " | Semester: ";

        if (sqlite3_column_type(stmt, 4) == SQLITE_NULL) {
            cout << "Not assigned";
        } else {
            cout << sqlite3_column_int(stmt, 4);
        }

        cout << '\n';
    }

    if (!found && rc == SQLITE_DONE) {
        cout << "No subjects found.\n";
    }

    if (rc != SQLITE_DONE) {
        cout << "Subject listing failed.\n";
    }

    sqlite3_finalize(stmt);
}

void updateSubjectMetadata(sqlite3* db) {
    int subjectId;
    double credits;
    int semester;

    cout << "Enter subject ID: ";

    if (!(cin >> subjectId) || subjectId <= 0) {
        cout << "Invalid subject ID.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    cout << "Enter credits: ";

    if (!(cin >> credits) || credits <= 0) {
        cout << "Enter valid positive credits.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    cout << "Enter semester (1-8): ";

    if (!(cin >> semester) || semester < 1 || semester > 8) {
        cout << "Invalid semester.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "UPDATE subjects SET credits = ?, semester = ? "
        "WHERE subject_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_double(stmt, 1, credits);
    sqlite3_bind_int(stmt, 2, semester);
    sqlite3_bind_int(stmt, 3, subjectId);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        logAuditEvent(db, "SYSTEM", "SUBJECT_UPDATED", std::to_string(subjectId), "Credits: " + std::to_string(credits) + ", Semester: " + std::to_string(semester));
        cout << "Subject metadata updated successfully.\n";
    } else if (rc == SQLITE_DONE) {
        cout << "Subject ID not found or metadata unchanged.\n";
    } else {
        cout << "Update failed: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

