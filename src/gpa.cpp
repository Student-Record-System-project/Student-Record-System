
#include "gpa.h"

#include <cmath>

int convertMarksToGradePoints(double marks) {
    if (!std::isfinite(marks) || marks < 0.0 || marks > 100.0)
        return -1;

    if (marks >= 90.0) return 10;
    if (marks >= 80.0) return 9;
    if (marks >= 70.0) return 8;
    if (marks >= 60.0) return 7;
    if (marks >= 50.0) return 6;
    if (marks >= 40.0) return 5;

    return 0;
}

bool calculateSemesterGPA(
    sqlite3* db,
    const std::string& studentId,
    int semester,
    double& gpa
) {
    if (semester < 1 || semester > 8)
        return false;

    const char* sql =
        "SELECT s.credits, g.marks "
        "FROM grades g "
        "JOIN subjects s ON s.subject_id = g.subject_id "
        "WHERE g.student_id = ? AND s.semester = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(
        stmt, 1, studentId.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_int(stmt, 2, semester);

    double totalWeightedPoints = 0.0;
    double totalCredits = 0.0;
    bool valid = true;
    int rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            valid = false;
            break;
        }

        double credits = sqlite3_column_double(stmt, 0);
        double marks = sqlite3_column_double(stmt, 1);
        int gradePoints = convertMarksToGradePoints(marks);

        if (!std::isfinite(credits) || credits <= 0 ||
            gradePoints < 0) {
            valid = false;
            break;
        }

        totalWeightedPoints += credits * gradePoints;
        totalCredits += credits;
    }

    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
        valid = false;

    sqlite3_finalize(stmt);

    if (!valid || totalCredits <= 0)
        return false;

    gpa = totalWeightedPoints / totalCredits;
    return true;
}
