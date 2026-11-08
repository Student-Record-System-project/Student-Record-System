#include "audit.h"

#include <iostream>
#include <string>

int main() {
    sqlite3* db = nullptr;

    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        std::cerr << "FAIL: Could not open test database.\n";
        if (db != nullptr) sqlite3_close(db);
        return 1;
    }

    if (!initializeAudit(db)) {
        std::cerr << "FAIL: Audit table initialization.\n";
        sqlite3_close(db);
        return 1;
    }
    std::cout << "PASS: Audit table initialization.\n";

    if (!logAuditEvent(db, "TEST_USER", "TEST_ACTION",
                       "TEST_TARGET", "Test details")) {
        std::cerr << "FAIL: Audit event insertion.\n";
        sqlite3_close(db);
        return 1;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* query =
        "SELECT COUNT(*) FROM audit_logs WHERE actor='TEST_USER' "
        "AND action='TEST_ACTION';";

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK ||
        sqlite3_step(stmt) != SQLITE_ROW ||
        sqlite3_column_int(stmt, 0) != 1) {
        std::cerr << "FAIL: Audit event was not stored correctly.\n";
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_finalize(stmt);
    std::cout << "PASS: Audit event insertion.\n";

    const char* oldEvent =
        "INSERT INTO audit_logs(event_time, actor, action) "
        "VALUES('2020-01-01 00:00:00', 'TEST_USER', 'OLD_EVENT');";

    if (sqlite3_exec(db, oldEvent, nullptr, nullptr, nullptr) != SQLITE_OK ||
        !initializeAudit(db)) {
        std::cerr << "FAIL: Retention test setup.\n";
        sqlite3_close(db);
        return 1;
    }

    query = "SELECT COUNT(*) FROM audit_logs WHERE action='OLD_EVENT';";

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK ||
        sqlite3_step(stmt) != SQLITE_ROW ||
        sqlite3_column_int(stmt, 0) != 0) {
        std::cerr << "FAIL: Old audit event was not deleted.\n";
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_finalize(stmt);
    std::cout << "PASS: Five-year audit log retention.\n";

    sqlite3_close(db);
    std::cout << "All audit tests passed.\n";
    return 0;
}
