
#ifndef GPA_H
#define GPA_H

#include <string>
#include "sqlite3.h"

int convertMarksToGradePoints(double marks);

bool calculateSemesterGPA(
    sqlite3* db,
    const std::string& studentId,
    int semester,
    double& gpa
);

bool calculateCGPA(
    sqlite3* db,
    const std::string& studentId,
    double& cgpa
);

#endif
