
#include <iostream>
#include "sqlite3.h"

int main() {
    sqlite3* db = nullptr;

    int rc = sqlite3_open(":memory:", &db);

    if (rc != SQLITE_OK) {
        std::cerr << "FAIL: SQLite database connection: "
                  << (db ? sqlite3_errmsg(db) : "No database handle")
                  << std::endl;

        if (db) sqlite3_close(db);
        return 1;
    }

    std::cout << "PASS: In-memory SQLite connection." << std::endl;

    const char* sql =
        "CREATE TABLE students ("
        "student_id TEXT PRIMARY KEY, "
        "name TEXT NOT NULL, "
        "email TEXT);";

    rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "FAIL: Students table creation: "
                  << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    std::cout << "PASS: Students table creation." << std::endl;

    rc = sqlite3_exec(
        db,
        "INSERT INTO students VALUES "
        "('S001', 'Alice', 'alice@example.com');",
        nullptr, nullptr, nullptr
    );

    if (rc != SQLITE_OK) {
        std::cerr << "FAIL: Student insertion: "
                  << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    std::cout << "PASS: Student insertion." << std::endl;

    sqlite3_stmt* stmt = nullptr;

    rc = sqlite3_prepare_v2(
        db,
        "SELECT name FROM students WHERE student_id = 'S001';",
        -1, &stmt, nullptr
    );

    if (rc != SQLITE_OK || sqlite3_step(stmt) != SQLITE_ROW) {
        std::cerr << "FAIL: Student retrieval." << std::endl;
        if (stmt) sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    std::cout << "PASS: Student retrieval: "
              << sqlite3_column_text(stmt, 0) << std::endl;

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << "All SQLite tests passed." << std::endl;
    return 0;
}
