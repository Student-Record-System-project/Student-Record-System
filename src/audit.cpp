#include "audit.h"

bool initializeAudit(sqlite3* db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS audit_logs ("
        "audit_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "event_time TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "actor TEXT NOT NULL,"
        "action TEXT NOT NULL,"
        "target TEXT,"
        "details TEXT"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_audit_time "
        "ON audit_logs(event_time);"
        "CREATE INDEX IF NOT EXISTS idx_audit_actor "
        "ON audit_logs(actor);";

    if (sqlite3_exec(db, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        return false;
    }

    const char* retentionSql =
        "DELETE FROM audit_logs "
        "WHERE event_time < datetime('now', '-5 years');";

    return sqlite3_exec(db, retentionSql, nullptr, nullptr, nullptr)
           == SQLITE_OK;
}

bool logAuditEvent(
    sqlite3* db,
    const std::string& actor,
    const std::string& action,
    const std::string& target,
    const std::string& details
) {
    const char* sql =
        "INSERT INTO audit_logs(actor, action, target, details) "
        "VALUES(?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, actor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, target.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, details.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);
    return success;
}
