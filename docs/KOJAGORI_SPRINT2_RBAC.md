# Sprint 2: Role-Based Access Control

## 1. Objective
Implement centralized authorization for Admin, Faculty, Staff, and
Student roles using a permission-based access-control model.

## 2. Implementation
- Added `src/rbac.h`.
- Introduced a centralized `Permission` enum with 17 permissions.
- Added `normalizeRole()` for case-insensitive role matching.
- Implemented `hasPermission()` for centralized authorization.
- Integrated permission checks into menu visibility and feature
  execution paths.
- Unknown roles are denied by default.

## 3. Permission Matrix

| Permission | Admin | Faculty | Staff | Student |
|---|---|---|---|---|
| Create student | Yes | No | No | No |
| View students | Yes | Yes | Yes | No |
| Search students | Yes | Yes | Yes | No |
| Update student | Yes | Yes | No | No |
| Delete student | Yes | Yes | No | No |
| Manage users | Yes | No | No | No |
| Manage subjects | Yes | No | No | No |
| Add grades | Yes | Yes | No | No |
| View all grades | Yes | Yes | No | No |
| View own grades | Yes | Yes | No | Yes |
| Record attendance | Yes | Yes | No | No |
| Update attendance | Yes | Yes | No | No |
| View all attendance | Yes | Yes | Yes | No |
| View own attendance | Yes | Yes | No | Yes |
| Generate academic report | Yes | Yes | No | No |
| View grade sheet | Yes | Yes | Yes | Yes |
| View attendance summary | Yes | Yes | Yes | Yes |

## 4. Security Behavior
- Admin receives all 17 permissions.
- Faculty receives academic operation permissions, excluding
  user and subject management and student creation.
- Staff receives read-oriented student and attendance permissions.
- Students receive own-grade and own-attendance permissions,
  grade-sheet access, and attendance-summary access.
- Role names are normalized to lowercase before evaluation.
- Unknown roles receive no permissions.
- Feature execution checks are integrated wherever implemented,
  independently of menu visibility.

## 5. Test Evidence

Test file: `tests/rbac_test.cpp`

Compile command:
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude -Isrc tests/rbac_test.cpp -o rbac_test.exe

Execution command:
./rbac_test.exe

Verified results:
- All 17 permissions checked for each of the four roles.
- Case-insensitive role matching passed.
- Unknown-role default-deny checks passed.

Test output:
PASS: All role permission checks.
PASS: Case-insensitive role checks.
PASS: Unknown roles are denied all permissions.
All RBAC tests passed.

## 6. Limitations
- These tests validate the centralized permission function.
- They do not independently prove every application workflow is
  protected against unauthorized access.
- Full end-to-end authorization testing should be performed for
  each protected feature.

## 7. Conclusion
The centralized RBAC implementation passed its permission-matrix,
case-insensitivity, and unknown-role denial tests.
