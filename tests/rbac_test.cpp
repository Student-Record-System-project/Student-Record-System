#include "rbac.h"

#include <iostream>

int main() {
    const Permission permissions[] = {
        Permission::CreateStudent,
        Permission::ViewStudents,
        Permission::SearchStudents,
        Permission::UpdateStudent,
        Permission::DeleteStudent,
        Permission::ManageUsers,
        Permission::ManageSubjects,
        Permission::AddGrades,
        Permission::ViewAllGrades,
        Permission::ViewOwnGrades,
        Permission::RecordAttendance,
        Permission::UpdateAttendance,
        Permission::ViewAllAttendance,
        Permission::ViewOwnAttendance,
        Permission::GenerateAcademicReport,
        Permission::ViewGradeSheet,
        Permission::ViewAttendanceSummary
    };

    const bool expected[][17] = {
        // Admin
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        // Faculty
        {0,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1},
        // Staff
        {0,1,1,0,0,0,0,0,0,0,0,0,1,0,0,1,1},
        // Student
        {0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1}
    };

    const char* roles[] = {"admin", "faculty", "staff", "student"};

    for (int r = 0; r < 4; ++r) {
        for (int p = 0; p < 17; ++p) {
            if (hasPermission(roles[r], permissions[p]) != expected[r][p]) {
                std::cerr << "FAIL: " << roles[r]
                          << " permission check " << p << '\n';
                return 1;
            }
        }
    }

    std::cout << "PASS: All role permission checks.\n";

    if (!hasPermission("ADMIN", Permission::ManageUsers) ||
        !hasPermission("Faculty", Permission::AddGrades)) {
        std::cerr << "FAIL: Case-insensitive role checks.\n";
        return 1;
    }

    std::cout << "PASS: Case-insensitive role checks.\n";

    for (Permission permission : permissions) {
        if (hasPermission("unknown", permission)) {
            std::cerr << "FAIL: Unknown role was granted permission.\n";
            return 1;
        }
    }

    std::cout << "PASS: Unknown roles are denied all permissions.\n";
    std::cout << "All RBAC tests passed.\n";
    return 0;
}
