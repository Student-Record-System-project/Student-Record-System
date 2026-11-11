# RBAC Maintenance Guide

The centralized permission definitions are maintained in src/rbac.h.

When introducing a new permission:

1. Add the permission to the Permission enum.
2. Define its access policy for each role.
3. Add corresponding test coverage.
4. Update the permission documentation.
5. Review affected application operations.
