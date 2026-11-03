#include "reports.h"
#include "attendance.h"
#include "rbac.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>
#include <cctype>

using namespace std;

namespace {

string trim(const string& value) {
    size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

string currentUtcIso8601() {
    time_t now = time(nullptr);
    tm gm_tm{};
#ifdef _WIN32
    gmtime_s(&gm_tm, &now);
#else
    gmtime_r(&now, &gm_tm);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &gm_tm);
    return string(buf);
}

inline const char* safeText(const unsigned char* text) {
    return text ? reinterpret_cast<const char*>(text) : "";
}

void printGradeSheetSection(sqlite3* db, const string& student_id, bool* out_had_rows) {
    cout << "\n--- Grade Sheet ---\n";

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT s.subject_code, s.subject_name, g.marks "
        "FROM grades g "
        "JOIN subjects s ON s.subject_id = g.subject_id "
        "WHERE g.student_id = ? "
        "ORDER BY s.subject_code ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        if (out_had_rows) *out_had_rows = false;
        return;
    }

    sqlite3_bind_text(stmt, 1, student_id.c_str(), -1, SQLITE_TRANSIENT);

    double totalMarks = 0.0;
    int count = 0;
    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!found) {
            found = true;
            cout << "Subject          Marks\n";
        }
        const char* code = safeText(sqlite3_column_text(stmt, 0));
        const char* name = safeText(sqlite3_column_text(stmt, 1));
        double marks = sqlite3_column_double(stmt, 2);

        totalMarks += marks;
        count++;

        cout << code << " " << name << "    "
             << fixed << setprecision(2) << marks << '\n';
    }
    sqlite3_finalize(stmt);

    if (!found) {
        cout << "No grade records found for student '" << student_id << "'.\n";
        if (out_had_rows) *out_had_rows = false;
        return;
    }

    double average = (count > 0) ? (totalMarks / count) : 0.0;
    cout << "---------------------------------------------\n";
    cout << "Overall average: " << fixed << setprecision(2) << average << '\n';

    if (out_had_rows) *out_had_rows = true;
}

void printAttendanceSummarySection(sqlite3* db, const string& student_id, bool* out_had_rows) {
    cout << "\n--- Attendance Summary ---\n";

    struct SubjectBreakdown {
        int id;
        string code;
        string name;
        int present;
        int total;
    };

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT s.subject_id, s.subject_code, s.subject_name, "
        "       SUM(CASE WHEN a.status = 'Present' THEN 1 ELSE 0 END) AS present_count, "
        "       COUNT(a.attendance_id) AS total_count "
        "FROM attendance a "
        "JOIN subjects s ON s.subject_id = a.subject_id "
        "WHERE a.student_id = ? "
        "GROUP BY s.subject_id, s.subject_code, s.subject_name "
        "ORDER BY s.subject_code ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        if (out_had_rows) *out_had_rows = false;
        return;
    }

    sqlite3_bind_text(stmt, 1, student_id.c_str(), -1, SQLITE_TRANSIENT);

    vector<SubjectBreakdown> breakdowns;
    int overallPresent = 0;
    int overallTotal = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SubjectBreakdown sb;
        sb.id = sqlite3_column_int(stmt, 0);
        sb.code = safeText(sqlite3_column_text(stmt, 1));
        sb.name = safeText(sqlite3_column_text(stmt, 2));
        sb.present = sqlite3_column_int(stmt, 3);
        sb.total = sqlite3_column_int(stmt, 4);

        overallPresent += sb.present;
        overallTotal += sb.total;
        breakdowns.push_back(sb);
    }
    sqlite3_finalize(stmt);

    if (breakdowns.empty()) {
        cout << "No attendance records found for student '" << student_id << "'.\n";
        if (out_had_rows) *out_had_rows = false;
        return;
    }

    cout << "Per-subject breakdown (subject_code, present/total, percentage):\n";
    for (const auto& sb : breakdowns) {
        bool ok = false;
        double pct = calculateAttendancePercentage(db, student_id, sb.id, &ok);
        cout << "    " << sb.code << " " << sb.name << "  "
             << sb.present << "/" << sb.total << "  "
             << fixed << setprecision(2) << pct << "%\n";
    }

    bool overallOk = false;
    double overallPct = calculateAttendancePercentage(db, student_id, -1, &overallOk);

    cout << "Overall attendance: " << fixed << setprecision(2) << overallPct << "%   "
         << "(present:" << overallPresent << " total:" << overallTotal << ")\n";
    cout << "'late' policy: counted as " << (kLateCountsAsPresent ? "present" : "absent") << '\n';

    if (out_had_rows) *out_had_rows = true;
}

} // anonymous namespace

bool initializeReports(sqlite3* db) {
    (void)db;
    // Reserved for future report logging or auditing
    return true;
}

bool studentExists(sqlite3* db, const string& student_id, string* out_name) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT name FROM students WHERE student_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, student_id.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        if (out_name) {
            *out_name = safeText(sqlite3_column_text(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

bool generateAcademicReport(sqlite3* db, const string& role, const string& username) {
    if (!hasPermission(role, Permission::GenerateAcademicReport)) {
        cout << "Access denied.\n";
        return false;
    }

    string studentId;
    string normRole = normalizeRole(role);

    if (normRole == "student") {
        studentId = username;
    } else {
        cout << "Student ID: ";
        if (!getline(cin, studentId)) return false;
        studentId = trim(studentId);
        if (studentId.empty()) {
            cout << "Error: student ID is required.\n";
            return false;
        }
    }

    string studentName;
    if (!studentExists(db, studentId, &studentName)) {
        cout << "Error: no student found with ID '" << studentId << "'.\n";
        return false;
    }

    cout << "\n=== Academic Report ===\n";
    cout << "Student: " << studentId << " — " << studentName << '\n';
    cout << "Generated: " << currentUtcIso8601() << '\n';

    bool hadGrades = false;
    bool hadAttendance = false;

    printGradeSheetSection(db, studentId, &hadGrades);
    printAttendanceSummarySection(db, studentId, &hadAttendance);

    if (!hadGrades) {
        cout << "Note: No grade records available for this student.\n";
    }
    if (!hadAttendance) {
        cout << "Note: No attendance records available for this student.\n";
    }
    if (!hadGrades && !hadAttendance) {
        cout << "Report complete but contains no data.\n";
    }

    return true;
}

bool viewGradeSheet(sqlite3* db, const string& role, const string& username) {
    if (!hasPermission(role, Permission::ViewGradeSheet)) {
        cout << "Access denied.\n";
        return false;
    }

    string studentId;
    string normRole = normalizeRole(role);

    if (normRole == "student") {
        studentId = username;
    } else {
        cout << "Student ID: ";
        if (!getline(cin, studentId)) return false;
        studentId = trim(studentId);
        if (studentId.empty()) {
            cout << "Error: student ID is required.\n";
            return false;
        }
    }

    string studentName;
    if (!studentExists(db, studentId, &studentName)) {
        cout << "Error: no student found with ID '" << studentId << "'.\n";
        return false;
    }

    cout << "\n=== Grade Sheet ===\n";
    cout << "Student: " << studentId << " — " << studentName << '\n';
    cout << "Generated: " << currentUtcIso8601() << '\n';

    bool hadRows = false;
    printGradeSheetSection(db, studentId, &hadRows);
    return true;
}

bool viewAttendanceSummary(sqlite3* db, const string& role, const string& username) {
    if (!hasPermission(role, Permission::ViewAttendanceSummary)) {
        cout << "Access denied.\n";
        return false;
    }

    string studentId;
    string normRole = normalizeRole(role);

    if (normRole == "student") {
        studentId = username;
    } else {
        cout << "Student ID: ";
        if (!getline(cin, studentId)) return false;
        studentId = trim(studentId);
        if (studentId.empty()) {
            cout << "Error: student ID is required.\n";
            return false;
        }
    }

    string studentName;
    if (!studentExists(db, studentId, &studentName)) {
        cout << "Error: no student found with ID '" << studentId << "'.\n";
        return false;
    }

    cout << "\n=== Attendance Summary ===\n";
    cout << "Student: " << studentId << " — " << studentName << '\n';
    cout << "Generated: " << currentUtcIso8601() << '\n';

    bool hadRows = false;
    printAttendanceSummarySection(db, studentId, &hadRows);
    return true;
}
