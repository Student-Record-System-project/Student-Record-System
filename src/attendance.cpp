#include "attendance.h"

#include "audit.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <cctype>
#include <algorithm>

using namespace std;

namespace {

string trim(const string& value) {
    size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool validDate(const string& date) {
    if (date.size() != 10) return false;
    if (date[4] != '-' || date[7] != '-') return false;
    for (size_t i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) continue;
        if (!isdigit(static_cast<unsigned char>(date[i]))) return false;
    }
    int year = stoi(date.substr(0, 4));
    int month = stoi(date.substr(5, 2));
    int day = stoi(date.substr(8, 2));
    if (year < 1900 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    return true;
}

string normalizeStatus(const string& input) {
    string lower = input;
    transform(lower.begin(), lower.end(), lower.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });
    if (lower == "present") return "Present";
    if (lower == "absent") return "Absent";
    if (lower == "late") return "Late";
    return "";
}

} // anonymous namespace

bool initializeAttendance(sqlite3* db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS attendance ("
        "attendance_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id TEXT NOT NULL REFERENCES students(student_id) ON DELETE CASCADE,"
        "subject_id INTEGER NOT NULL REFERENCES subjects(subject_id) ON DELETE CASCADE,"
        "date TEXT NOT NULL,"
        "status TEXT NOT NULL CHECK(status IN ('Present', 'Absent', 'Late')),"
        "UNIQUE(student_id, subject_id, date)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_attendance_student ON attendance(student_id);"
        "CREATE INDEX IF NOT EXISTS idx_attendance_subject ON attendance(subject_id);"
        "CREATE INDEX IF NOT EXISTS idx_attendance_student_subject ON attendance(student_id, subject_id);";

    char* error = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        cerr << "Attendance table setup failed: "
             << (error ? error : sqlite3_errmsg(db)) << '\n';
        sqlite3_free(error);
        return false;
    }
    return true;
}

double calculateAttendancePercentage(int presentCount, int totalCount, int lateCount) {
    if (totalCount <= 0) return 0.0;
    int effectivePresent = presentCount + (kLateCountsAsPresent ? lateCount : 0);
    return (static_cast<double>(effectivePresent) / totalCount) * 100.0;
}

double calculateAttendancePercentage(sqlite3* db, const std::string& studentId, int subjectId, bool* ok) {
    sqlite3_stmt* stmt = nullptr;
    string sql;
    if (subjectId > 0) {
        sql = "SELECT status, COUNT(*) FROM attendance WHERE student_id = ? AND subject_id = ? GROUP BY status;";
    } else {
        sql = "SELECT status, COUNT(*) FROM attendance WHERE student_id = ? GROUP BY status;";
    }

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        if (ok) *ok = false;
        return 0.0;
    }

    sqlite3_bind_text(stmt, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);
    if (subjectId > 0) {
        sqlite3_bind_int(stmt, 2, subjectId);
    }

    int present = 0;
    int late = 0;
    int total = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* s = sqlite3_column_text(stmt, 0);
        int count = sqlite3_column_int(stmt, 1);
        if (s) {
            string status = reinterpret_cast<const char*>(s);
            if (status == "Present") present += count;
            else if (status == "Late") late += count;
        }
        total += count;
    }
    sqlite3_finalize(stmt);

    if (ok) *ok = (total > 0);
    return calculateAttendancePercentage(present, total, late);
}

double calculateAttendancePercentage(sqlite3* db, const std::string& studentId, int subjectId) {
    return calculateAttendancePercentage(db, studentId, subjectId, nullptr);
}

void recordAttendance(sqlite3* db) {
    cout << "\n--- Record Attendance ---\n";
    cout << "Student ID: ";
    string studentId;
    if (!getline(cin, studentId)) return;
    studentId = trim(studentId);
    if (studentId.empty()) {
        cout << "Error: student ID is required.\n";
        return;
    }

    // Verify student exists
    sqlite3_stmt* checkStudent = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM students WHERE student_id = ?;", -1, &checkStudent, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }
    sqlite3_bind_text(checkStudent, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(checkStudent);
    sqlite3_finalize(checkStudent);
    if (rc != SQLITE_ROW) {
        cout << "Error: student with ID '" << studentId << "' does not exist.\n";
        return;
    }

    cout << "Subject ID: ";
    string subjectStr;
    if (!getline(cin, subjectStr)) return;
    subjectStr = trim(subjectStr);
    int subjectId = 0;
    try {
        size_t pos = 0;
        subjectId = stoi(subjectStr, &pos);
        if (pos != subjectStr.size() || subjectId <= 0) {
            cout << "Error: enter a valid positive subject ID.\n";
            return;
        }
    } catch (...) {
        cout << "Error: enter a valid numeric subject ID.\n";
        return;
    }

    // Verify subject exists
    sqlite3_stmt* checkSubject = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM subjects WHERE subject_id = ?;", -1, &checkSubject, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }
    sqlite3_bind_int(checkSubject, 1, subjectId);
    rc = sqlite3_step(checkSubject);
    sqlite3_finalize(checkSubject);
    if (rc != SQLITE_ROW) {
        cout << "Error: subject with ID " << subjectId << " does not exist.\n";
        return;
    }

    cout << "Date (YYYY-MM-DD): ";
    string dateStr;
    if (!getline(cin, dateStr)) return;
    dateStr = trim(dateStr);
    if (!validDate(dateStr)) {
        cout << "Error: date must be in YYYY-MM-DD format.\n";
        return;
    }

    cout << "Status (Present/Absent/Late): ";
    string statusStr;
    if (!getline(cin, statusStr)) return;
    statusStr = trim(statusStr);
    string normStatus = normalizeStatus(statusStr);
    if (normStatus.empty()) {
        cout << "Error: status must be 'Present', 'Absent', or 'Late'.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO attendance (student_id, subject_id, date, status) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }
    sqlite3_bind_text(stmt, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_text(stmt, 3, dateStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, normStatus.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        logAuditEvent(db, "SYSTEM", "ATTENDANCE_RECORDED", studentId + "/" + std::to_string(subjectId) + "/" + dateStr, "Status: " + normStatus);
        cout << "Attendance recorded successfully.\n";
    } else {
        cout << "Error: could not record attendance. An attendance record for this student, subject, and date may already exist.\n"
             << "Database message: " << sqlite3_errmsg(db) << '\n';
    }
    sqlite3_finalize(stmt);
}

void updateAttendance(sqlite3* db) {
    cout << "\n--- Update Attendance ---\n";
    cout << "Student ID: ";
    string studentId;
    if (!getline(cin, studentId)) return;
    studentId = trim(studentId);
    if (studentId.empty()) {
        cout << "Error: student ID is required.\n";
        return;
    }

    cout << "Subject ID: ";
    string subjectStr;
    if (!getline(cin, subjectStr)) return;
    subjectStr = trim(subjectStr);
    int subjectId = 0;
    try {
        size_t pos = 0;
        subjectId = stoi(subjectStr, &pos);
        if (pos != subjectStr.size() || subjectId <= 0) {
            cout << "Error: enter a valid positive subject ID.\n";
            return;
        }
    } catch (...) {
        cout << "Error: enter a valid numeric subject ID.\n";
        return;
    }

    cout << "Date (YYYY-MM-DD): ";
    string dateStr;
    if (!getline(cin, dateStr)) return;
    dateStr = trim(dateStr);
    if (!validDate(dateStr)) {
        cout << "Error: date must be in YYYY-MM-DD format.\n";
        return;
    }

    // Check if record exists
    sqlite3_stmt* check = nullptr;
    const char* checkSql = "SELECT status FROM attendance WHERE student_id = ? AND subject_id = ? AND date = ?;";
    if (sqlite3_prepare_v2(db, checkSql, -1, &check, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }
    sqlite3_bind_text(check, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(check, 2, subjectId);
    sqlite3_bind_text(check, 3, dateStr.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(check);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(check);
        cout << "Error: no attendance record found for student '" << studentId << "', subject " << subjectId << ", date " << dateStr << ".\n";
        return;
    }
    const unsigned char* curStatus = sqlite3_column_text(check, 0);
    string currentStatus = curStatus ? reinterpret_cast<const char*>(curStatus) : "Unknown";
    sqlite3_finalize(check);

    cout << "Current status: " << currentStatus << '\n';
    cout << "New Status (Present/Absent/Late): ";
    string newStatusStr;
    if (!getline(cin, newStatusStr)) return;
    newStatusStr = trim(newStatusStr);
    string normStatus = normalizeStatus(newStatusStr);
    if (normStatus.empty()) {
        cout << "Error: status must be 'Present', 'Absent', or 'Late'.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE attendance SET status = ? WHERE student_id = ? AND subject_id = ? AND date = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }
    sqlite3_bind_text(stmt, 1, normStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, studentId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, subjectId);
    sqlite3_bind_text(stmt, 4, dateStr.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        logAuditEvent(db, "SYSTEM", "ATTENDANCE_UPDATED", studentId + "/" + std::to_string(subjectId) + "/" + dateStr, "New status: " + normStatus);
        cout << "Attendance updated successfully.\n";
    } else {
        cout << "Error: could not update attendance.\n"
             << "Database message: " << sqlite3_errmsg(db) << '\n';
    }
}

void viewAttendance(sqlite3* db, const std::string& username, const std::string& role) {
    string studentId;
    string normRole = role;
    for (char& c : normRole) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    if (normRole == "student") {
        studentId = username;
    } else {
        cout << "\nStudent ID: ";
        if (!getline(cin, studentId)) return;
        studentId = trim(studentId);
        if (studentId.empty()) {
            cout << "Error: student ID is required.\n";
            return;
        }
    }

    cout << "Subject ID (press Enter for all subjects): ";
    string subjectStr;
    if (!getline(cin, subjectStr)) return;
    subjectStr = trim(subjectStr);
    int filterSubjectId = 0;
    if (!subjectStr.empty()) {
        try {
            size_t pos = 0;
            filterSubjectId = stoi(subjectStr, &pos);
            if (pos != subjectStr.size() || filterSubjectId <= 0) {
                cout << "Error: enter a valid positive subject ID.\n";
                return;
            }
        } catch (...) {
            cout << "Error: enter a valid numeric subject ID.\n";
            return;
        }
    }

    sqlite3_stmt* stmt = nullptr;
    string sql;
    if (filterSubjectId > 0) {
        sql = "SELECT a.date, s.subject_code, s.subject_name, a.status "
              "FROM attendance a "
              "JOIN subjects s ON s.subject_id = a.subject_id "
              "WHERE a.student_id = ? AND a.subject_id = ? "
              "ORDER BY a.date ASC, s.subject_code ASC;";
    } else {
        sql = "SELECT a.date, s.subject_code, s.subject_name, a.status "
              "FROM attendance a "
              "JOIN subjects s ON s.subject_id = a.subject_id "
              "WHERE a.student_id = ? "
              "ORDER BY a.date ASC, s.subject_code ASC;";
    }

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_text(stmt, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);
    if (filterSubjectId > 0) {
        sqlite3_bind_int(stmt, 2, filterSubjectId);
    }

    cout << "\n--- Attendance Records for " << studentId << " ---\n";
    int totalCount = 0;
    int presentCount = 0;
    int absentCount = 0;
    int lateCount = 0;
    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        totalCount++;
        const unsigned char* dateCol = sqlite3_column_text(stmt, 0);
        const unsigned char* codeCol = sqlite3_column_text(stmt, 1);
        const unsigned char* nameCol = sqlite3_column_text(stmt, 2);
        const unsigned char* statusCol = sqlite3_column_text(stmt, 3);

        string status = statusCol ? reinterpret_cast<const char*>(statusCol) : "";
        if (status == "Present") presentCount++;
        else if (status == "Absent") absentCount++;
        else if (status == "Late") lateCount++;

        cout << (dateCol ? reinterpret_cast<const char*>(dateCol) : "") << " | "
             << (codeCol ? reinterpret_cast<const char*>(codeCol) : "") << " ("
             << (nameCol ? reinterpret_cast<const char*>(nameCol) : "") << ") | "
             << status << '\n';
    }
    sqlite3_finalize(stmt);

    if (!found) {
        cout << "No attendance records found.\n";
        return;
    }

    double percentage = calculateAttendancePercentage(presentCount, totalCount, lateCount);

    cout << "\n--- Attendance Summary ---\n"
         << "Total Sessions: " << totalCount << '\n'
         << "Present       : " << presentCount << '\n'
         << "Absent        : " << absentCount << '\n'
         << "Late          : " << lateCount << '\n'
         << "Attendance Percentage: " << fixed << setprecision(2) << percentage << "%\n";
}

void viewAttendance(sqlite3* db) {
    viewAttendance(db, "", "admin");
}
