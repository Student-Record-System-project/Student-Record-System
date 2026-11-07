#ifndef AUDIT_H
#define AUDIT_H

#include <string>
#include "sqlite3.h"

bool initializeAudit(sqlite3* db);

bool logAuditEvent(
    sqlite3* db,
    const std::string& actor,
    const std::string& action,
    const std::string& target,
    const std::string& details
);

#endif
