#ifndef GRADES_H
#define GRADES_H

#include <string>
#include "sqlite3.h"

bool initializeGrades(sqlite3* db);
void addGrade(sqlite3* db, const std::string& actor = "SYSTEM");
void listGradesForStudent(sqlite3* db);

#endif
