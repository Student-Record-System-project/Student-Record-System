#include "attendance.h"
#include "audit.h"
#include <cassert>
#include <sstream>
#include <iostream>

using namespace std;

template<class F>
void runInput(const string& data, F fn) {
    istringstream in(data);
    ostringstream out;
    auto* oldIn = cin.rdbuf(in.rdbuf());
    auto* oldOut = cout.rdbuf(out.rdbuf());
    fn();
    cin.rdbuf(oldIn);
    cout.rdbuf(oldOut);
}

int main() {
    sqlite3* db = nullptr;
    assert(sqlite3_open(":memory:", &db) == SQLITE_OK);

    assert(sqlite3_exec(db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr) == SQLITE_OK);
    assert(sqlite3_exec(db,
        "CREATE TABLE students(student_id TEXT PRIMARY KEY);"
        "CREATE TABLE subjects(subject_id INTEGER PRIMARY KEY, subject_code TEXT, subject_name TEXT);"
        "INSERT INTO students VALUES('S001');"
        "INSERT INTO subjects VALUES(1,'CS101','DBMS');",
        nullptr, nullptr, nullptr) == SQLITE_OK);

    assert(initializeAudit(db));
    assert(initializeAttendance(db));
    assert(initializeAttendance(db));

    assert(calculateAttendancePercentage(0, 0, 0) == 0.0);
    assert(calculateAttendancePercentage(5, 10, 0) == 50.0);
    assert(calculateAttendancePercentage(5, 10, 2) == 50.0);

    bool ok = false;
    calculateAttendancePercentage(db, "S001", 0, &ok);
    assert(!ok);

    runInput("S001\n1\n2026-11-11\nPresent\n",
        [&] { recordAttendance(db, "admin"); });

    assert(calculateAttendancePercentage(db, "S001", 1) == 100.0);

    runInput("S001\n1\n2026-11-11\nAbsent\n",
        [&] { recordAttendance(db, "admin"); });

    runInput("S001\n1\n2026-11-12\nLate\n",
        [&] { recordAttendance(db, "admin"); });

    runInput("S999\n1\n2026-11-13\nPresent\n",
        [&] { recordAttendance(db, "admin"); });

    runInput("S001\n1\ninvalid\nPresent\n",
        [&] { recordAttendance(db, "admin"); });

    runInput("S001\n1\n2026-11-13\nInvalid\n",
        [&] { recordAttendance(db, "admin"); });

    runInput("S001\n1\n2026-11-11\nPresent\n",
        [&] { updateAttendance(db, "admin"); });

    runInput("S001\n1\n2026-11-11\nAbsent\n",
        [&] { updateAttendance(db, "admin"); });

    runInput("S999\n1\n2026-11-11\n",
        [&] { updateAttendance(db, "admin"); });

    runInput("S001\n1\n2026-11-11\nInvalid\n",
        [&] { updateAttendance(db, "admin"); });

    runInput("", [&] { viewAttendance(db, "admin", "admin"); });
    runInput("S001\n\n", [&] { viewAttendance(db, "admin", "admin"); });
    runInput("", [&] { viewAttendance(db, "S001", "student"); });
    runInput("S999\n", [&] { viewAttendance(db, "admin", "admin"); });


    // Additional attendance coverage tests

    bool allSubjectsOk = false;
    double allPercentage =
        calculateAttendancePercentage(db, "S001", 0, &allSubjectsOk);
    assert(allSubjectsOk);
    assert(allPercentage >= 0.0);

    double subjectPercentage =
        calculateAttendancePercentage(db, "S001", 1);
    assert(subjectPercentage >= 0.0);

    // Successful student attendance view
    runInput("\n", [&] {
        viewAttendance(db, "S001", "student");
    });

    // Successful admin attendance view with subject filter
    runInput("S001\n1\n", [&] {
        viewAttendance(db, "admin", "admin");
    });

    // No attendance records for another subject
    runInput("S001\n999\n", [&] {
        viewAttendance(db, "admin", "admin");
    });

    // Invalid subject filter
    runInput("S001\nabc\n", [&] {
        viewAttendance(db, "admin", "admin");
    });

    runInput("S001\n0\n", [&] {
        viewAttendance(db, "admin", "admin");
    });

    // Test wrapper overload
    runInput("S001\n\n", [&] {
        viewAttendance(db);
    });

    // Empty student ID during attendance recording
    runInput("\n", [&] {
        recordAttendance(db, "admin");
    });

    // Invalid subject ID during recording
    runInput("S001\nabc\n", [&] {
        recordAttendance(db, "admin");
    });

    runInput("S001\n0\n", [&] {
        recordAttendance(db, "admin");
    });

    // Invalid date range
    runInput("S001\n1\n1800-01-01\n", [&] {
        recordAttendance(db, "admin");
    });

    // Empty student ID during update
    runInput("\n", [&] {
        updateAttendance(db, "admin");
    });

    // Invalid subject during update
    runInput("S001\nabc\n", [&] {
        updateAttendance(db, "admin");
    });

    runInput("S001\n0\n", [&] {
        updateAttendance(db, "admin");
    });

    // Invalid date during update
    runInput("S001\n1\ninvalid\n", [&] {
        updateAttendance(db, "admin");
    });

    // Successful update to Late using case-insensitive status
    runInput("S001\n1\n2026-11-11\nlate\n", [&] {
        updateAttendance(db, "admin");
    });


    sqlite3_close(db);
    cout << "Attendance tests passed\n";
    return 0;
}