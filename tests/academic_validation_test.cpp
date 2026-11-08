#include <iostream>
#include "sqlite3.h"
#include "subjects.h"
#include "grades.h"

bool execute(sqlite3* db, const char* sql, int expected,
             const char* description) {
    char* error = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);

    if (rc != expected) {
        std::cerr << "FAIL: " << description
                  << " (SQLite code " << rc << ")";
        if (error) std::cerr << ": " << error;
        std::cerr << '\n';
        sqlite3_free(error);
        return false;
    }

    sqlite3_free(error);
    std::cout << "PASS: " << description << '\n';
    return true;
}

bool hasColumn(sqlite3* db, const char* column) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(subjects);",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        if (name && std::string(
                reinterpret_cast<const char*>(name)) == column) {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

int main() {
    sqlite3* db = nullptr;

    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        std::cerr << "FAIL: Could not open in-memory database.\n";
        if (db) sqlite3_close(db);
        return 1;
    }

    bool success = true;

    success &= initializeSubjects(db);
    std::cout << (success ? "PASS" : "FAIL")
              << ": Subject table initialization\n";

    success &= initializeSubjects(db);
    std::cout << (success ? "PASS" : "FAIL")
              << ": Repeated subject initialization\n";

    success &= hasColumn(db, "credits");
    std::cout << (hasColumn(db, "credits") ? "PASS" : "FAIL")
              << ": Credits column exists\n";

    success &= hasColumn(db, "semester");
    std::cout << (hasColumn(db, "semester") ? "PASS" : "FAIL")
              << ": Semester column exists\n";

    success &= execute(db,
        "CREATE TABLE students ("
        "student_id TEXT PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "email TEXT);",
        SQLITE_OK, "Student table creation");

    success &= initializeGrades(db);
    std::cout << "Grade table initialization: "
              << (success ? "completed" : "failed") << '\n';

    success &= execute(db,
        "INSERT INTO students VALUES"
        "('S001','Alice','alice@example.com'),"
        "('S002','Bob','bob@example.com');",
        SQLITE_OK, "Insert test students");

    success &= execute(db,
        "INSERT INTO subjects(subject_code,subject_name,credits,semester)"
        " VALUES('CS101','Database Systems',4,3),"
        "('CS102','Programming',3,2);",
        SQLITE_OK, "Insert test subjects");

    success &= execute(db,
        "INSERT INTO subjects(subject_code,subject_name)"
        " VALUES('CS101','Duplicate Subject');",
        SQLITE_CONSTRAINT, "Reject duplicate subject code");

    success &= execute(db,
        "INSERT INTO grades(student_id,subject_id,marks)"
        " VALUES('S001',1,0);",
        SQLITE_OK, "Accept minimum marks (0)");

    success &= execute(db,
        "INSERT INTO grades(student_id,subject_id,marks)"
        " VALUES('S002',2,100);",
        SQLITE_OK, "Accept maximum marks (100)");

    success &= execute(db,
        "INSERT INTO grades(student_id,subject_id,marks)"
        " VALUES('S001',2,-1);",
        SQLITE_CONSTRAINT, "Reject negative marks");

    success &= execute(db,
        "INSERT INTO grades(student_id,subject_id,marks)"
        " VALUES('S002',1,100.1);",
        SQLITE_CONSTRAINT, "Reject marks above 100");

    success &= execute(db,
        "INSERT INTO grades(student_id,subject_id,marks)"
        " VALUES('S001',1,50);",
        SQLITE_CONSTRAINT, "Reject duplicate student-subject grade");

    success &= execute(db,
        "INSERT INTO grades(student_id,subject_id,marks)"
        " VALUES('S999',1,50);",
        SQLITE_CONSTRAINT, "Reject grade for nonexistent student");

    success &= execute(db,
        "INSERT INTO grades(student_id,subject_id,marks)"
        " VALUES('S001',999,50);",
        SQLITE_CONSTRAINT, "Reject grade for nonexistent subject");

    if (success) {
        std::cout << "\nAll academic database validation tests passed.\n";
    } else {
        std::cerr << "\nSome academic database validation tests failed.\n";
    }

    sqlite3_close(db);
    return success ? 0 : 1;
}
