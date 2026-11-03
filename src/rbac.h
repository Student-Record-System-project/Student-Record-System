#ifndef RBAC_H
#define RBAC_H

#include <string>
#include <algorithm>
#include <cctype>

enum class Permission {
    CreateStudent,
    ViewStudents,
    SearchStudents,
    UpdateStudent,
    DeleteStudent,
    ManageUsers,
    ManageSubjects,
    AddGrades,
    ViewAllGrades,
    ViewOwnGrades
};

inline std::string normalizeRole(std::string role) {
    std::transform(role.begin(), role.end(), role.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    return role;
}

inline bool hasPermission(
    const std::string& inputRole,
    Permission permission
) {
    const std::string role = normalizeRole(inputRole);

    if (role == "admin") return true;

    if (role == "faculty") {
        switch (permission) {
            case Permission::ViewStudents:
            case Permission::SearchStudents:
            case Permission::UpdateStudent:
            case Permission::DeleteStudent:
            case Permission::AddGrades:
            case Permission::ViewAllGrades:
            case Permission::ViewOwnGrades:
                return true;
            default:
                return false;
        }
    }

    if (role == "staff") {
        return permission == Permission::ViewStudents ||
               permission == Permission::SearchStudents;
    }

    if (role == "student") {
        return permission == Permission::ViewOwnGrades;
    }

    return false;
}

#endif