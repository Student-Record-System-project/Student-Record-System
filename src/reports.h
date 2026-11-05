#ifndef REPORTS_H
#define REPORTS_H

#include <string>
#include "sqlite3.h"

// Reserved for future report logging or auditing migrations.
bool initializeReports(sqlite3* db);

// TC-RPT-01: Full academic report (grades + attendance) for one student
bool generateAcademicReport(sqlite3* db,
                            const std::string& role,
                            const std::string& username);

// TC-RPT-02: Grade sheet only
bool viewGradeSheet(sqlite3* db,
                    const std::string& role,
                    const std::string& username);

// TC-RPT-02: Attendance summary only
bool viewAttendanceSummary(sqlite3* db,
                           const std::string& role,
                           const std::string& username);

// Helper: Check student existence, returns true and fills out_name if found
bool studentExists(sqlite3* db,
                   const std::string& student_id,
                   std::string* out_name);

#endif // REPORTS_H
