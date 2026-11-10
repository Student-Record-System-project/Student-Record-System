
#include <iostream>
#include <cmath>
#include "sqlite3.h"
#include "gpa.h"

int passed = 0;
int failed = 0;

void check(bool condition, const char* description) {
    if (condition) {
        std::cout << "PASS: " << description << '\n';
        ++passed;
    } else {
        std::cerr << "FAIL: " << description << '\n';
        ++failed;
    }
}

bool setupDatabase(sqlite3*& db) {
    if (sqlite3_open(":memory:", &db) != SQLITE_OK)
        return false;

    const char* sql =
        "CREATE TABLE students (student_id TEXT PRIMARY KEY);"
        "CREATE TABLE subjects ("
        "subject_id INTEGER PRIMARY KEY,"
        "credits REAL NOT NULL,"
        "semester INTEGER NOT NULL);"
        "CREATE TABLE grades ("
        "student_id TEXT NOT NULL,"
        "subject_id INTEGER NOT NULL,"
        "marks REAL NOT NULL);"
        "INSERT INTO students VALUES ('S001');"
        "INSERT INTO subjects VALUES (1,4,1),(2,3,1),(3,2,2);"
        "INSERT INTO grades VALUES "
        "('S001',1,95),('S001',2,85),('S001',3,75);";

    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

int main() {
    check(convertMarksToGradePoints(95) == 10, "Marks 95 -> 10 points");
    check(convertMarksToGradePoints(85) == 9, "Marks 85 -> 9 points");
    check(convertMarksToGradePoints(75) == 8, "Marks 75 -> 8 points");
    check(convertMarksToGradePoints(65) == 7, "Marks 65 -> 7 points");
    check(convertMarksToGradePoints(55) == 6, "Marks 55 -> 6 points");
    check(convertMarksToGradePoints(45) == 5, "Marks 45 -> 5 points");
    check(convertMarksToGradePoints(35) == 0, "Marks 35 -> 0 points");
    check(convertMarksToGradePoints(-1) == -1, "Reject negative marks");
    check(convertMarksToGradePoints(101) == -1, "Reject marks above 100");
    check(convertMarksToGradePoints(NAN) == -1, "Reject NaN marks");
    check(convertMarksToGradePoints(INFINITY) == -1, "Reject infinite marks");

    sqlite3* db = nullptr;
    check(setupDatabase(db), "Create test database and academic records");

    if (db) {
        double gpa = -1;
        check(calculateSemesterGPA(db, "S001", 1, gpa),
              "Calculate semester 1 GPA");

        check(std::abs(gpa - (67.0 / 7.0)) < 0.0001,
              "Verify credit-weighted semester GPA");

        double cgpa = -1;
        check(calculateCGPA(db, "S001", cgpa),
              "Calculate cumulative CGPA");

        check(std::abs(cgpa - (83.0 / 9.0)) < 0.0001,
              "Verify credit-weighted CGPA");

        check(!calculateSemesterGPA(db, "S001", 9, gpa),
              "Reject invalid semester");

        check(!calculateSemesterGPA(db, "S999", 1, gpa),
              "Reject student without semester records");

        check(!calculateCGPA(db, "S999", cgpa),
              "Reject student without academic records");

        sqlite3_close(db);
    }

    std::cout << "\nPassed: " << passed
              << "\nFailed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
