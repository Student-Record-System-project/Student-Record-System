# Sprint 2: Role-Based Access Control

## Objective
Implement centralized authorization for Admin, Faculty,
Staff, and Student roles.

## Implementation
- Added src/rbac.h.
- Introduced a Permission enum.
- Added normalizeRole() for case-insensitive role handling.
- Added hasPermission() for centralized authorization.
- Integrated permission checks into the application menu
  and feature execution paths.
- Replaced inconsistent admin/teacher checks.

## Roles
- Admin: Full access.
- Faculty: Student record updates, grade operations,
  and student/grade viewing.
- Staff: Student record viewing and searching.
- Student: Viewing their own grades only.

## Security
- Unknown roles are denied by default.
- Restricted menu options are not displayed.
- Feature execution checks permissions independently.

## Testing
Testing results will be recorded after execution.

## Integration
Branch: feature/kojagori-rbac