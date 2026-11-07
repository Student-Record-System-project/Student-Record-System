#ifndef SUBJECTS_H
#define SUBJECTS_H

#include <string>
#include "sqlite3.h"

bool initializeSubjects(sqlite3* db);
void addSubject(sqlite3* db, const std::string& actor = "SYSTEM");
void updateSubjectMetadata(sqlite3* db, const std::string& actor = "SYSTEM");
void listSubjects(sqlite3* db);

#endif
