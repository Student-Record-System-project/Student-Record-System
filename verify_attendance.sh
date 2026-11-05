
#!/usr/bin/env bash
# ==============================================================================
# verify_attendance.sh - Acceptance test script for SRS-F-012, 013, 014
#
# Tests:
#   TC-ATT-01: Record attendance
#   TC-ATT-02: Update attendance
#   TC-ATT-03: Calculate attendance percentage
#   RBAC: Student cannot update attendance
# ==============================================================================

set -euo pipefail

echo "=========================================================="
echo "Starting Attendance Module Acceptance Verification"
echo "=========================================================="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ -f "./student_system" ]; then
    APP="./student_system"
elif [ -f "./student_system.exe" ]; then
    APP="./student_system.exe"
else
    echo "Error: student_system binary not found."
    exit 1
fi

TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

echo "1. Initializing isolated test environment..."
mkdir -p "$TEMP_DIR/data"
cp "$APP" "$TEMP_DIR/"
APP_BIN="$(basename "$APP")"

cd "$TEMP_DIR"

echo "2. Seeding initial accounts and subject..."

cat << 'EOF' | "./$APP_BIN" > seed.log 2>&1
admin
AdminPass123!
admin
AdminPass123!
6
faculty1
FacultyPass123!
Faculty
6
STU001
StudentPass123!
Student
1
STU001
Alice Smith
alice@example.com
7
CS101
Computer Science
0
EOF

if [ ! -f "data/student_records.db" ]; then
    echo "FAIL: Database was not created."
    cat seed.log
    exit 1
fi

echo "PASS: Initial data seeded."

echo "3. Testing attendance recording..."

cat << 'EOF' | "./$APP_BIN" > record.log 2>&1
faculty1
FacultyPass123!
12
STU001
1
2026-11-01
Present
12
STU001
1
2026-11-02
Absent
0
EOF

if grep -q "Attendance recorded successfully" record.log; then
    echo "PASS: Attendance recording."
else
    echo "FAIL: Attendance recording."
    cat record.log
    exit 1
fi

echo "4. Testing attendance percentage and RBAC..."

cat << 'EOF' | "./$APP_BIN" > student_view.log 2>&1
STU001
StudentPass123!
14

13
0
EOF

if grep -q "50.00%" student_view.log; then
    echo "PASS: Initial attendance percentage is 50%."
else
    echo "FAIL: Initial attendance percentage."
    cat student_view.log
    exit 1
fi

if grep -q "Access denied" student_view.log; then
    echo "PASS: Student update access correctly denied."
else
    echo "FAIL: Student RBAC verification."
    cat student_view.log
    exit 1
fi

echo "5. Testing attendance update..."

cat << 'EOF' | "./$APP_BIN" > update.log 2>&1
faculty1
FacultyPass123!
13
STU001
1
2026-11-02
Present
0
EOF

if grep -q "Attendance updated successfully" update.log; then
    echo "PASS: Attendance update."
else
    echo "FAIL: Attendance update."
    cat update.log
    exit 1
fi

echo "6. Verifying updated attendance percentage..."

cat << 'EOF' | "./$APP_BIN" > student_after_update.log 2>&1
STU001
StudentPass123!
14

0
EOF

if grep -q "100.00%" student_after_update.log; then
    echo "PASS: Updated attendance percentage is 100%."
else
    echo "FAIL: Updated attendance percentage."
    cat student_after_update.log
    exit 1
fi

echo "=========================================================="
echo "ALL ATTENDANCE ACCEPTANCE TESTS PASSED"
echo "=========================================================="

exit 0
