#include "grades.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;

bool initializeGrades(sqlite3* db) {
    // Enable foreign-key checks on this database connection.
    char* error = nullptr;
    int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        cerr << "Could not enable foreign keys: "
             << (error ? error : sqlite3_errmsg(db)) << '\n';
        sqlite3_free(error);
        return false;
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS grades ("
        "grade_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id TEXT NOT NULL REFERENCES students(student_id),"
        "subject_id INTEGER NOT NULL REFERENCES subjects(subject_id),"
        "marks REAL NOT NULL CHECK(marks >= 0 AND marks <= 100),"
        "UNIQUE(student_id, subject_id)"
        ");";

    rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        cerr << "Grade table setup failed: "
             << (error ? error : sqlite3_errmsg(db)) << '\n';
        sqlite3_free(error);
        return false;
    }

    return true;
}

void addGrade(sqlite3* db) {
    string studentId;
    string subjectInput;
    double marks;

    cout << "Student ID: ";
    getline(cin, studentId);

    if (studentId.empty()) {
        cout << "Student ID is required.\n";
        return;
    }

    cout << "Subject ID: ";
    getline(cin, subjectInput);

    if (subjectInput.empty()) {
        cout << "Subject ID is required.\n";
        return;
    }

    try {
        size_t pos = 0;
        int subjectId = stoi(subjectInput, &pos);
        if (pos != subjectInput.size() || subjectId <= 0) {
            cout << "Enter a valid positive subject ID.\n";
            return;
        }

        cout << "Marks (0-100): ";
        if (!(cin >> marks)) {
            cout << "Invalid marks input.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (marks < 0 || marks > 100) {
            cout << "Marks must be between 0 and 100.\n";
            return;
        }

        sqlite3_stmt* stmt = nullptr;
        const char* sql =
            "INSERT INTO grades(student_id, subject_id, marks) VALUES(?, ?, ?);";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            cout << "Database error: " << sqlite3_errmsg(db) << '\n';
            return;
        }

        sqlite3_bind_text(stmt, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, subjectId);
        sqlite3_bind_double(stmt, 3, marks);

        int rc = sqlite3_step(stmt);

        if (rc == SQLITE_DONE) {
            cout << "Grade added successfully.\n";
        } else {
            cout << "Could not add grade. Check that the student and subject "
                    "exist and that this student does not already have a grade "
                    "for this subject.\n"
                 << "Database message: " << sqlite3_errmsg(db) << '\n';
        }

        sqlite3_finalize(stmt);
    } catch (...) {
        cout << "Enter a valid numeric subject ID.\n";
    }
}

void listGradesForStudent(sqlite3* db) {
    string studentId;

    cout << "Student ID: ";
    getline(cin, studentId);

    if (studentId.empty()) {
        cout << "Student ID is required.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT s.subject_code, s.subject_name, g.marks "
        "FROM grades g "
        "JOIN subjects s ON s.subject_id = g.subject_id "
        "WHERE g.student_id = ? "
        "ORDER BY s.subject_code;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_text(stmt, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);

    cout << "\n--- Grades for " << studentId << " ---\n";

    bool found = false;
    int rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        found = true;
        cout << "Subject: "
             << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))
             << " | "
             << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))
             << " | Marks: " << sqlite3_column_double(stmt, 2) << '\n';
    }

    if (!found && rc == SQLITE_DONE) {
        cout << "No grades found for this student.\n";
    } else if (rc != SQLITE_DONE) {
        cout << "Grade listing failed: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}
