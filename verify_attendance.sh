#!/usr/bin/env bash
# ==============================================================================
# verify_attendance.sh - Acceptance test script for SRS-F-012, 013, 014
#
# Tests:
#   TC-ATT-01 (SRS-F-012): Record attendance for student and subject
#   TC-ATT-02 (SRS-F-013): Update existing attendance record
#   TC-ATT-03 (SRS-F-014): Calculate and display attendance percentage
#   RBAC Check: Ensure student role attempting updateAttendance is rejected
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
    echo "Error: student_system binary not found. Build the project first."
    exit 1
fi

TEMP_DIR="$(mktemp -d 2>/dev/null || mktemp -d -t 'srs_attendance_test')"
trap 'rm -rf "$TEMP_DIR"' EXIT

echo "1. Initializing isolated test environment at: $TEMP_DIR"
mkdir -p "$TEMP_DIR/data"
cp "$APP" "$TEMP_DIR/"
APP_BIN="$(basename "$APP")"

cd "$TEMP_DIR"

# ------------------------------------------------------------------------------
# Phase 1: Seed initial admin, faculty, student, and subject via CLI
# ------------------------------------------------------------------------------
echo "2. Seeding initial accounts and subject via CLI (Admin setup)..."

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
    echo "FAIL: Database data/student_records.db was not created."
    cat seed.log
    exit 1
fi

echo "   Seeding completed successfully."

# ------------------------------------------------------------------------------
# Phase 2: TC-ATT-01 (SRS-F-012) - Record Attendance (Faculty)
# ------------------------------------------------------------------------------
echo "3. Testing TC-ATT-01: Faculty records attendance (Present on Day 1, Absent on Day 2)..."

cat << 'EOF' | "./$APP_BIN" > record.log 2>&1
faculty1
FacultyPass123!
11
STU001
1
2026-11-01
Present
11
STU001
1
2026-11-02
Absent
0
EOF

if grep -q "Attendance recorded successfully" record.log; then
    echo "   PASS: TC-ATT-01 - Attendance successfully recorded."
else
    echo "   FAIL: TC-ATT-01 - Could not record attendance."
    cat record.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 3: TC-ATT-03 (SRS-F-014) - Attendance percentage before update (50.00%)
#          and RBAC verification (Student role attempting Update is rejected)
# ------------------------------------------------------------------------------
echo "4. Testing TC-ATT-03 & RBAC: Student views percentage and attempts update..."

cat << 'EOF' | "./$APP_BIN" > student_view.log 2>&1
STU001
StudentPass123!
13

12
0
EOF

# Verify initial percentage (1 Present, 1 Absent = 50.00%)
if grep -q "50.00%" student_view.log; then
    echo "   PASS: TC-ATT-03 - Initial attendance percentage correctly calculated as 50.00%."
else
    echo "   FAIL: TC-ATT-03 - Initial attendance percentage 50.00% not found."
    cat student_view.log
    exit 1
fi

# Verify student update is rejected with "Access denied"
if grep -q "Access denied" student_view.log; then
    echo "   PASS: RBAC - Student role attempting Update Attendance was rejected (Access denied)."
else
    echo "   FAIL: RBAC - Student role was not properly denied for Update Attendance."
    cat student_view.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 4: TC-ATT-02 (SRS-F-013) - Update Attendance (Faculty)
# ------------------------------------------------------------------------------
echo "5. Testing TC-ATT-02: Faculty updates attendance (Change Day 2 from Absent to Present)..."

cat << 'EOF' | "./$APP_BIN" > update.log 2>&1
faculty1
FacultyPass123!
12
STU001
1
2026-11-02
Present
0
EOF

if grep -q "Attendance updated successfully" update.log; then
    echo "   PASS: TC-ATT-02 - Attendance record updated successfully."
else
    echo "   FAIL: TC-ATT-02 - Could not update attendance."
    cat update.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 5: TC-ATT-03 (SRS-F-014) - Verify updated percentage (100.00%)
# ------------------------------------------------------------------------------
echo "6. Testing TC-ATT-03: Verifying updated percentage after update (100.00%)..."

cat << 'EOF' | "./$APP_BIN" > student_after_update.log 2>&1
STU001
StudentPass123!
13

0
EOF

if grep -q "100.00%" student_after_update.log; then
    echo "   PASS: TC-ATT-03 - Updated attendance percentage correctly calculated as 100.00%."
else
    echo "   FAIL: TC-ATT-03 - Updated attendance percentage 100.00% not found."
    cat student_after_update.log
    exit 1
fi

echo "=========================================================="
echo "ALL ATTENDANCE ACCEPTANCE TESTS PASSED (TC-ATT-01, 02, 03)"
echo "=========================================================="
exit 0
