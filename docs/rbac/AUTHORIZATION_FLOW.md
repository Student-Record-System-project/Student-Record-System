# Authorization Flow

1. The user authenticates.
2. The application identifies the user's assigned role.
3. The requested operation specifies a permission.
4. The RBAC layer checks the role against that permission.
5. Unauthorized operations are rejected.

Authorization checks should protect operations, not only menu visibility.
