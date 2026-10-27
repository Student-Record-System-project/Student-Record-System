#ifndef GRADES_H
#define GRADES_H

#include "sqlite3.h"

bool initializeGrades(sqlite3* db);
void addGrade(sqlite3* db);
void listGradesForStudent(sqlite3* db);

#endif
