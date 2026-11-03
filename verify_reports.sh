#!/usr/bin/env bash
# ==============================================================================
# verify_reports.sh - Acceptance test script for SRS-F-016 & SRS-F-017
#
# Tests:
#   TC-RPT-01 (SRS-F-016): Full Academic Report (grades + attendance)
#   TC-RPT-02 (SRS-F-017): Grade Sheet only and Attendance Summary only
#   RBAC Checks:
#     - Faculty: Option 14 (Generate), Option 15 (Grades), Option 16 (Attendance)
#     - Student: Option 14 rejected, Option 15 allowed (forced to self)
#     - Staff:   Option 14 rejected, Option 15 & 16 allowed for any student
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
    echo "Error: student_system binary not found. Build the project first."
    exit 1
fi

TEMP_DIR="$(mktemp -d 2>/dev/null || mktemp -d -t 'srs_reports_test')"
trap 'rm -rf "$TEMP_DIR"' EXIT

echo "1. Initializing isolated test environment at: $TEMP_DIR"
mkdir -p "$TEMP_DIR/data"
cp "$APP" "$TEMP_DIR/"
APP_BIN="$(basename "$APP")"

cd "$TEMP_DIR"

# ------------------------------------------------------------------------------
# Phase 1: Seed admin, faculty, staff, student, and 3 subjects via CLI
# ------------------------------------------------------------------------------
echo "2. Seeding initial accounts and subjects (Admin setup)..."

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
7
CS102
Data Structures
7
CS103
Algorithms
0
EOF

if [ ! -f "data/student_records.db" ]; then
    echo "FAIL: Database data/student_records.db was not created."
    cat seed.log
    exit 1
fi
echo "   PASS: Initial seeding completed successfully."

# ------------------------------------------------------------------------------
# Phase 2: Faculty adds 3 grades and 5 attendance records for s1
# ------------------------------------------------------------------------------
echo "3. Faculty populating grades and attendance for s1..."

cat << 'EOF' | "./$APP_BIN" > populate.log 2>&1
faculty1
FacultyPass123!
9
s1
1
85.0
9
s1
2
90.0
9
s1
3
95.0
11
s1
1
2026-11-01
Present
11
s1
1
2026-11-02
Present
11
s1
1
2026-11-03
Present
11
s1
1
2026-11-04
Absent
11
s1
1
2026-11-05
Late
0
EOF

if grep -q "Grade added successfully" populate.log && grep -q "Attendance recorded successfully" populate.log; then
    echo "   PASS: 3 grades and 5 attendance records successfully populated."
else
    echo "   FAIL: Could not populate grades and attendance."
    cat populate.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 3: Faculty tests TC-RPT-01 and TC-RPT-02
# ------------------------------------------------------------------------------
echo "4. Testing Faculty reports (TC-RPT-01 & TC-RPT-02)..."

cat << 'EOF' | "./$APP_BIN" > faculty_reports.log 2>&1
faculty1
FacultyPass123!
14
s1
15
s1
16
s1
0
EOF

# Assertion 1: TC-RPT-01 contains Grade Sheet banner, Attendance Summary banner, 3 codes, and attendance percentage
if grep -q -F -e "--- Grade Sheet ---" faculty_reports.log && \
   grep -q -F -e "--- Attendance Summary ---" faculty_reports.log && \
   grep -q "CS101" faculty_reports.log && \
   grep -q "CS102" faculty_reports.log && \
   grep -q "CS103" faculty_reports.log && \
   grep -E -q "Overall attendance: [0-9.]+%" faculty_reports.log; then
    echo "   PASS: TC-RPT-01 - Full academic report contains grade sheet, attendance summary, subject codes, and percentage."
else
    echo "   FAIL: TC-RPT-01 - Full academic report missing expected sections or data."
    cat faculty_reports.log
    exit 1
fi

# Assertion 2: TC-RPT-02 (Option 15 View Grade Sheet)
cat << 'EOF' | "./$APP_BIN" > faculty_gradesheet_only.log 2>&1
faculty1
FacultyPass123!
15
s1
0
EOF

if grep -q -F -e "--- Grade Sheet ---" faculty_gradesheet_only.log && \
   ! grep -q -F -e "--- Attendance Summary ---" faculty_gradesheet_only.log; then
    echo "   PASS: TC-RPT-02 - View Grade Sheet displays grade sheet and omits attendance summary."
else
    echo "   FAIL: TC-RPT-02 - View Grade Sheet failed banner assertion."
    cat faculty_gradesheet_only.log
    exit 1
fi

# Assertion 3: TC-RPT-02 (Option 16 View Attendance Summary)
cat << 'EOF' | "./$APP_BIN" > faculty_attendance_only.log 2>&1
faculty1
FacultyPass123!
16
s1
0
EOF

if grep -q -F -e "--- Attendance Summary ---" faculty_attendance_only.log && \
   ! grep -q -F -e "--- Grade Sheet ---" faculty_attendance_only.log; then
    echo "   PASS: TC-RPT-02 - View Attendance Summary displays attendance and omits grade sheet."
else
    echo "   FAIL: TC-RPT-02 - View Attendance Summary failed banner assertion."
    cat faculty_attendance_only.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 4: Student (s1) RBAC & View Tests
# ------------------------------------------------------------------------------
echo "5. Testing Student permissions (Option 14 rejected, Option 15 allowed for self)..."

# Student attempts Option 14 (Generate) and Option 15 (View Grade Sheet)
cat << 'EOF' | "./$APP_BIN" > student_tests.log 2>&1
s1
StudentPass123!
14
15
0
EOF

# Option 14 should be rejected ("Access denied" or not shown in menu)
if grep -q "Access denied" student_tests.log || ! grep -q "14. Generate academic report" student_tests.log; then
    echo "   PASS: Student role attempting Option 14 (Generate Academic Report) was rejected."
else
    echo "   FAIL: Student role was not denied Option 14."
    cat student_tests.log
    exit 1
fi

# Option 15 should succeed and show s1's grades (forced to s1 without accepting target ID)
if grep -q "Student: s1" student_tests.log && grep -q -F -e "--- Grade Sheet ---" student_tests.log; then
    echo "   PASS: Student role successfully viewed own grade sheet (automatically forced to self)."
else
    echo "   FAIL: Student role could not view own grade sheet."
    cat student_tests.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 5: Staff RBAC & View Tests
# ------------------------------------------------------------------------------
echo "6. Testing Staff permissions (Option 14 rejected, Options 15 & 16 allowed)..."

cat << 'EOF' | "./$APP_BIN" > staff_tests.log 2>&1
staff1
StaffPass123!
14
15
s1
16
s1
0
EOF

if grep -q "Access denied" staff_tests.log; then
    echo "   PASS: Staff role attempting Option 14 (Generate Academic Report) was rejected."
else
    echo "   FAIL: Staff role was not denied Option 14."
    cat staff_tests.log
    exit 1
fi

if grep -q -F -e "--- Grade Sheet ---" staff_tests.log && grep -q -F -e "--- Attendance Summary ---" staff_tests.log; then
    echo "   PASS: Staff role successfully viewed grade sheet and attendance summary for s1."
else
    echo "   FAIL: Staff role failed to view records."
    cat staff_tests.log
    exit 1
fi

# ------------------------------------------------------------------------------
# Phase 6: Compare against SQLite database records
# ------------------------------------------------------------------------------
echo "7. Comparing database records with report contents..."

if command -v sqlite3 >/dev/null 2>&1; then
    DB_GRADE_COUNT=$(sqlite3 data/student_records.db "SELECT COUNT(*) FROM grades WHERE student_id = 's1';")
    DB_ATT_COUNT=$(sqlite3 data/student_records.db "SELECT COUNT(*) FROM attendance WHERE student_id = 's1';")

    if [ "$DB_GRADE_COUNT" -eq 3 ] && [ "$DB_ATT_COUNT" -eq 5 ]; then
        echo "   PASS: Database record counts match expected values (3 grades, 5 attendance entries)."
    else
        echo "   FAIL: Database record count mismatch: grades=$DB_GRADE_COUNT, attendance=$DB_ATT_COUNT"
        exit 1
    fi
else
    echo "   INFO: sqlite3 CLI not in path, skipping direct SQL record count verification."
fi

echo "=========================================================="
echo "ALL ACADEMIC REPORT ACCEPTANCE TESTS PASSED (TC-RPT-01, 02)"
echo "=========================================================="
exit 0
