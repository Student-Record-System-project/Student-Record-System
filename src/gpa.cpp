
#include "gpa.h"

#include <cmath>

int convertMarksToGradePoints(double marks) {
    if (!std::isfinite(marks) || marks < 0.0 || marks > 100.0)
        return -1;

    if (marks >= 90.0) return 10;
    if (marks >= 80.0) return 9;
    if (marks >= 70.0) return 8;
    if (marks >= 60.0) return 7;
    if (marks >= 50.0) return 6;
    if (marks >= 40.0) return 5;

    return 0;
}
