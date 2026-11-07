#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "sqlite3.h"

using Clock = std::chrono::steady_clock;

constexpr int STUDENT_COUNT = 5000;
constexpr int SUBJECT_COUNT = 10;
constexpr int ATTENDANCE_DAYS = 5;
constexpr int WARMUP_RUNS = 5;
constexpr int TEST_RUNS = 100;

bool execute(sqlite3* db, const char* sql) {
    char* error = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);

    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: "
                  << (error ? error : sqlite3_errmsg(db)) << '\n';
        sqlite3_free(error);
        return false;
    }

    return true;
}

bool seedDatabase(sqlite3* db) {
    const char* schema =
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE students ("
        "student_id TEXT PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "email TEXT);"
        "CREATE TABLE subjects ("
        "subject_id INTEGER PRIMARY KEY,"
        "subject_code TEXT NOT NULL UNIQUE,"
        "subject_name TEXT NOT NULL);"
        "CREATE TABLE grades ("
        "grade_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id TEXT NOT NULL REFERENCES students(student_id),"
        "subject_id INTEGER NOT NULL REFERENCES subjects(subject_id),"
        "marks REAL NOT NULL,"
        "UNIQUE(student_id, subject_id));"
        "CREATE TABLE attendance ("
        "attendance_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id TEXT NOT NULL REFERENCES students(student_id),"
        "subject_id INTEGER NOT NULL REFERENCES subjects(subject_id),"
        "date TEXT NOT NULL,"
        "status TEXT NOT NULL CHECK(status IN "
        "('Present', 'Absent', 'Late')),"
        "UNIQUE(student_id, subject_id, date));"
        "CREATE INDEX idx_attendance_student "
        "ON attendance(student_id);"
        "CREATE INDEX idx_attendance_subject "
        "ON attendance(subject_id);"
        "CREATE INDEX idx_attendance_student_subject "
        "ON attendance(student_id, subject_id);";

    if (!execute(db, schema) || !execute(db, "BEGIN TRANSACTION;")) {
        return false;
    }

    sqlite3_stmt* studentStmt = nullptr;
    sqlite3_stmt* subjectStmt = nullptr;
    sqlite3_stmt* attendanceStmt = nullptr;

    const char* studentSql =
        "INSERT INTO students VALUES(?, ?, ?);";
    const char* subjectSql =
        "INSERT INTO subjects VALUES(?, ?, ?);";
    const char* attendanceSql =
        "INSERT INTO attendance(student_id, subject_id, date, status) "
        "VALUES(?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, studentSql, -1, &studentStmt, nullptr)
            != SQLITE_OK ||
        sqlite3_prepare_v2(db, subjectSql, -1, &subjectStmt, nullptr)
            != SQLITE_OK ||
        sqlite3_prepare_v2(db, attendanceSql, -1, &attendanceStmt, nullptr)
            != SQLITE_OK) {
        std::cerr << "Could not prepare data insertion statements.\n";
        sqlite3_finalize(studentStmt);
        sqlite3_finalize(subjectStmt);
        sqlite3_finalize(attendanceStmt);
        execute(db, "ROLLBACK;");
        return false;
    }

    bool success = true;

    for (int i = 1; i <= STUDENT_COUNT && success; ++i) {
        std::string id = "STU" + std::to_string(100000 + i).substr(1);
        std::string name = "Student " + std::to_string(i);
        std::string email = "student" + std::to_string(i) + "@test.edu";

        sqlite3_bind_text(studentStmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(studentStmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(studentStmt, 3, email.c_str(), -1, SQLITE_TRANSIENT);

        success = sqlite3_step(studentStmt) == SQLITE_DONE;
        sqlite3_reset(studentStmt);
        sqlite3_clear_bindings(studentStmt);
    }

    for (int i = 1; i <= SUBJECT_COUNT && success; ++i) {
        std::string code = "SUB" + std::to_string(i);
        std::string name = "Subject " + std::to_string(i);

        sqlite3_bind_int(subjectStmt, 1, i);
        sqlite3_bind_text(subjectStmt, 2, code.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(subjectStmt, 3, name.c_str(), -1, SQLITE_TRANSIENT);

        success = sqlite3_step(subjectStmt) == SQLITE_DONE;
        sqlite3_reset(subjectStmt);
        sqlite3_clear_bindings(subjectStmt);
    }

    const char* statuses[] = {"Present", "Absent", "Late"};

    for (int s = 1; s <= STUDENT_COUNT && success; ++s) {
        std::string id = "STU" + std::to_string(100000 + s).substr(1);

        for (int subject = 1; subject <= SUBJECT_COUNT && success; ++subject) {
            for (int day = 1; day <= ATTENDANCE_DAYS; ++day) {
                std::string date = "2026-10-0" + std::to_string(day);
                const char* status = statuses[(s + subject + day) % 3];

                sqlite3_bind_text(attendanceStmt, 1, id.c_str(),
                                  -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(attendanceStmt, 2, subject);
                sqlite3_bind_text(attendanceStmt, 3, date.c_str(),
                                  -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(attendanceStmt, 4, status,
                                  -1, SQLITE_TRANSIENT);

                success = sqlite3_step(attendanceStmt) == SQLITE_DONE;
                sqlite3_reset(attendanceStmt);
                sqlite3_clear_bindings(attendanceStmt);

                if (!success) break;
            }
        }
    }

    sqlite3_finalize(studentStmt);
    sqlite3_finalize(subjectStmt);
    sqlite3_finalize(attendanceStmt);

    if (!success) {
        std::cerr << "Synthetic data insertion failed: "
                  << sqlite3_errmsg(db) << '\n';
        execute(db, "ROLLBACK;");
        return false;
    }

    return execute(db, "COMMIT;");
}

bool runQuery(
    sqlite3* db,
    const char* sql,
    const std::function<void(sqlite3_stmt*)>& bind
) {
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Query preparation failed: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    bind(stmt);

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        // Consume returned values so the complete result is processed.
        (void)sqlite3_column_text(stmt, 0);
    }

    bool success = rc == SQLITE_DONE;

    if (!success) {
        std::cerr << "Query execution failed: "
                  << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    return success;
}

bool benchmark(
    sqlite3* db,
    const std::string& label,
    const char* sql,
    const std::function<void(sqlite3_stmt*)>& bind
) {
    std::vector<double> times;

    for (int i = 0; i < WARMUP_RUNS; ++i) {
        if (!runQuery(db, sql, bind)) return false;
    }

    for (int i = 0; i < TEST_RUNS; ++i) {
        auto start = Clock::now();

        if (!runQuery(db, sql, bind)) return false;

        auto end = Clock::now();

        double milliseconds =
            std::chrono::duration<double, std::milli>(end - start).count();

        times.push_back(milliseconds);
    }

    std::sort(times.begin(), times.end());

    double average = 0.0;
    for (double time : times) average += time;
    average /= times.size();

    double median = (times[49] + times[50]) / 2.0;
    double p95 = times[static_cast<size_t>(
        std::ceil(0.95 * times.size()) - 1)];

    std::cout << "\n" << label << '\n'
              << "  Average: " << average << " ms\n"
              << "  Median:  " << median << " ms\n"
              << "  P95:     " << p95 << " ms\n"
              << "  Maximum: " << times.back() << " ms\n";

    return true;
}

int main() {
    sqlite3* db = nullptr;

    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        std::cerr << "Could not create in-memory database.\n";
        if (db) sqlite3_close(db);
        return 1;
    }

    std::cout << "Student Record System Performance Benchmark\n"
              << "Synthetic students: " << STUDENT_COUNT << '\n'
              << "Subjects: " << SUBJECT_COUNT << '\n'
              << "Attendance records: "
              << STUDENT_COUNT * SUBJECT_COUNT * ATTENDANCE_DAYS << '\n'
              << "Warm-up runs: " << WARMUP_RUNS << '\n'
              << "Measured runs: " << TEST_RUNS << '\n';

    if (!seedDatabase(db)) {
        sqlite3_close(db);
        return 1;
    }

    std::cout << "\nData generation completed.\n";

    bool success = true;

    success &= benchmark(
        db,
        "1. Student listing",
        "SELECT student_id, name, email FROM students ORDER BY student_id;",
        [](sqlite3_stmt*) {}
    );

    success &= benchmark(
        db,
        "2. Student search",
        "SELECT student_id, name, email FROM students WHERE student_id = ?;",
        [](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, "STU02500", -1, SQLITE_STATIC);
        }
    );

    success &= benchmark(
        db,
        "3. Attendance summary",
        "SELECT status, COUNT(*) FROM attendance "
        "WHERE student_id = ? GROUP BY status;",
        [](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, "STU02500", -1, SQLITE_STATIC);
        }
    );

    std::cout << "\nPerformance target: P95 below 3000 ms.\n";
    std::cout << "Note: These are SQLite query timings, not full CLI response times.\n";

    sqlite3_close(db);
    return success ? 0 : 1;
}
