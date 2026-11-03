#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include <string>
#include "sqlite3.h"

// Institutional Policy / SRS-F-014: Late arrivals are logged for auditing
// but do not count as present toward the standard attendance percentage.
constexpr bool kLateCountsAsPresent = false;

bool initializeAttendance(sqlite3* db);
void recordAttendance(sqlite3* db);
void updateAttendance(sqlite3* db);
void viewAttendance(sqlite3* db, const std::string& username, const std::string& role);
void viewAttendance(sqlite3* db);

double calculateAttendancePercentage(int presentCount, int totalCount, int lateCount = 0);
double calculateAttendancePercentage(sqlite3* db, const std::string& studentId, int subjectId = 0);
double calculateAttendancePercentage(sqlite3* db, const std::string& studentId, int subjectId, bool* ok);

#endif // ATTENDANCE_H
