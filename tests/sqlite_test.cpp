#include <iostream>
#include "sqlite3.h"

int main() {
    std::cout << "Program started" << std::endl;

    sqlite3* db = nullptr;
    int rc = sqlite3_open("data/student_records.db", &db);

    std::cout << "sqlite3_open returned: " << rc << std::endl;

    if (rc == SQLITE_OK) {
        std::cout << "SQLite connected successfully!" << std::endl;

        char* error = nullptr;
        rc = sqlite3_exec(
            db,
            "CREATE TABLE IF NOT EXISTS students ("
            "student_id TEXT PRIMARY KEY, "
            "name TEXT NOT NULL, email TEXT);",
            nullptr, nullptr, &error
        );

        std::cout << "Table creation result: " << rc << std::endl;
        if (error) {
            std::cout << error << std::endl;
            sqlite3_free(error);
        }
    } else {
        std::cout << (db ? sqlite3_errmsg(db) : "No database handle")
                  << std::endl;
    }

    if (db) sqlite3_close(db);
    return rc == SQLITE_OK ? 0 : 1;
}