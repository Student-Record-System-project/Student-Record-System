#ifndef SUBJECTS_H
#define SUBJECTS_H

#include "sqlite3.h"

bool initializeSubjects(sqlite3* db);
void addSubject(sqlite3* db);
void listSubjects(sqlite3* db);

#endif
