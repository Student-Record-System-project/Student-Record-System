
#!/usr/bin/env bash
# ==============================================================================
# verify_reports.sh - Acceptance tests for SRS-F-016 and SRS-F-017
#
# Tests:
#   TC-RPT-01: Full Academic Report
#   TC-RPT-02: Grade Sheet and Attendance Summary
#   RBAC: Faculty, Student and Staff permissions
# ==============================================================================

set -euo pipefail

echo "=========================================================="
echo "Starting Academic Reports Acceptance Verification"
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

# ------------------------------------------------------------------------------
# Phase 1: Seed accounts, student and subjects
# ------------------------------------------------------------------------------

echo "2. Seeding initial accounts and subjects..."

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
staff1
StaffPass123!
Staff
6
s1
StudentPass123!
Student
1
s1
Student One
s1@example.com
7
CS101
Intro to Computer Science
3
1
7
CS102
Data Structures
3
1
7
CS103
Algorithms
3
1
0
EOF

if [ ! -f "data/student_records.db" ]; then
    echo "FAIL: Database was not created."
    cat seed.log
    exit 1
fi

echo "PASS: Initial accounts and subjects seeded."

# ------------------------------------------------------------------------------
# Phase 2: Faculty populates grades and attendance
# ------------------------------------------------------------------------------

echo "3. Faculty populating grades and attendance..."

cat << 'EOF' | "./$APP_BIN" > populate.log 2>&1
faculty1
FacultyPass123!
10
s1
1
85.0
10
s1
2
90.0
10
s1
3
95.0
12
s1
1
2026-11-01
Present
12
s1
1
2026-11-02
Present
12
s1
1
2026-11-03
Present
12
s1
1
2026-11-04
Absent
12
s1
1
2026-11-05
Late
0
EOF

if grep -q "Grade added successfully" populate.log && \
   grep -q "Attendance recorded successfully" populate.log; then
    echo "PASS: Grades and attendance populated."
else
    echo "FAIL: Could not populate grades and attendance."
    cat populate.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 3: Faculty report tests
# ------------------------------------------------------------------------------

echo "4. Testing Faculty academic reports..."

cat << 'EOF' | "./$APP_BIN" > faculty_reports.log 2>&1
faculty1
FacultyPass123!
15
s1
16
s1
17
s1
0
EOF

if grep -q -F -e "--- Grade Sheet ---" faculty_reports.log && \
   grep -q -F -e "--- Attendance Summary ---" faculty_reports.log && \
   grep -q "CS101" faculty_reports.log && \
   grep -q "CS102" faculty_reports.log && \
   grep -q "CS103" faculty_reports.log && \
   grep -E -q "Overall attendance: [0-9.]+%" faculty_reports.log; then
    echo "PASS: Full academic report contains expected sections and data."
else
    echo "FAIL: Full academic report assertion failed."
    cat faculty_reports.log
    exit 1
fi

# Grade Sheet only

cat << 'EOF' | "./$APP_BIN" > faculty_gradesheet_only.log 2>&1
faculty1
FacultyPass123!
16
s1
0
EOF

if grep -q -F -e "--- Grade Sheet ---" faculty_gradesheet_only.log && \
   ! grep -q -F -e "--- Attendance Summary ---" faculty_gradesheet_only.log; then
    echo "PASS: Faculty Grade Sheet only."
else
    echo "FAIL: Faculty Grade Sheet only assertion failed."
    cat faculty_gradesheet_only.log
    exit 1
fi

# Attendance Summary only

cat << 'EOF' | "./$APP_BIN" > faculty_attendance_only.log 2>&1
faculty1
FacultyPass123!
17
s1
0
EOF

if grep -q -F -e "--- Attendance Summary ---" faculty_attendance_only.log && \
   ! grep -q -F -e "--- Grade Sheet ---" faculty_attendance_only.log; then
    echo "PASS: Faculty Attendance Summary only."
else
    echo "FAIL: Faculty Attendance Summary assertion failed."
    cat faculty_attendance_only.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 4: Student RBAC tests
# ------------------------------------------------------------------------------

echo "5. Testing Student permissions..."

cat << 'EOF' | "./$APP_BIN" > student_tests.log 2>&1
s1
StudentPass123!
15
16
0
EOF

if grep -q "Access denied" student_tests.log || \
   ! grep -q "15. Generate academic report" student_tests.log; then
    echo "PASS: Student cannot generate full academic reports."
else
    echo "FAIL: Student was not denied full academic report access."
    cat student_tests.log
    exit 1
fi

if grep -q "Student: s1" student_tests.log && \
   grep -q -F -e "--- Grade Sheet ---" student_tests.log; then
    echo "PASS: Student can view their own grade sheet."
else
    echo "FAIL: Student could not view their own grade sheet."
    cat student_tests.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 5: Staff RBAC tests
# ------------------------------------------------------------------------------

echo "6. Testing Staff permissions..."

cat << 'EOF' | "./$APP_BIN" > staff_tests.log 2>&1
staff1
StaffPass123!
15
16
s1
17
s1
0
EOF

if grep -q "Access denied" staff_tests.log; then
    echo "PASS: Staff cannot generate full academic reports."
else
    echo "FAIL: Staff was not denied full academic report access."
    cat staff_tests.log
    exit 1
fi

if grep -q -F -e "--- Grade Sheet ---" staff_tests.log && \
   grep -q -F -e "--- Attendance Summary ---" staff_tests.log; then
    echo "PASS: Staff can view grade sheet and attendance summary."
else
    echo "FAIL: Staff report viewing failed."
    cat staff_tests.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 6: Database verification
# ------------------------------------------------------------------------------

echo "7. Comparing database record counts..."

if command -v sqlite3 >/dev/null 2>&1; then
    DB_GRADE_COUNT=$(sqlite3 data/student_records.db \
        "SELECT COUNT(*) FROM grades WHERE student_id = 's1';")

    DB_ATT_COUNT=$(sqlite3 data/student_records.db \
        "SELECT COUNT(*) FROM attendance WHERE student_id = 's1';")

    if [ "$DB_GRADE_COUNT" -eq 3 ] && [ "$DB_ATT_COUNT" -eq 5 ]; then
        echo "PASS: Database contains 3 grades and 5 attendance records."
    else
        echo "FAIL: Record count mismatch: grades=$DB_GRADE_COUNT, attendance=$DB_ATT_COUNT"
        exit 1
    fi
else
    echo "INFO: sqlite3 CLI unavailable; skipping direct SQL count verification."
fi

echo "=========================================================="
echo "ALL ACADEMIC REPORT ACCEPTANCE TESTS PASSED"
echo "=========================================================="

exit 0
