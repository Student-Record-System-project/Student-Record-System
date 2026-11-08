
# Student Record System - Test Cases

## 1. RBAC Unit Tests

| ID | Test case | Expected result | Actual result |
|---|---|---|---|
| TC-RBAC-01 | Verify role permissions | Correct permissions for each role | PASS |
| TC-RBAC-02 | Verify case-insensitive role names | Role names accepted regardless of case | PASS |
| TC-RBAC-03 | Verify unknown roles | All permissions denied | PASS |

## 2. Audit Unit Tests

| ID | Test case | Expected result | Actual result |
|---|---|---|---|
| TC-AUD-01 | Initialize audit table | Table initialized successfully | PASS |
| TC-AUD-02 | Insert audit event | Event inserted successfully | PASS |
| TC-AUD-03 | Verify five-year retention | Events older than five years removed | PASS |

## 3. Academic Database Validation Tests

| ID | Test case | Expected result | Actual result |
|---|---|---|---|
| TC-ACD-01 | Initialize subject table | Successful initialization | PASS |
| TC-ACD-02 | Repeat subject initialization | No initialization failure | PASS |
| TC-ACD-03 | Verify credits and semester columns | Both columns exist | PASS |
| TC-ACD-04 | Create student table | Successful creation | PASS |
| TC-ACD-05 | Initialize grade table | Successful initialization | PASS |
| TC-ACD-06 | Insert students and subjects | Records inserted | PASS |
| TC-ACD-07 | Insert duplicate subject code | Duplicate rejected | PASS |
| TC-ACD-08 | Insert minimum marks (0) | Accepted | PASS |
| TC-ACD-09 | Insert maximum marks (100) | Accepted | PASS |
| TC-ACD-10 | Insert negative marks | Rejected | PASS |
| TC-ACD-11 | Insert marks above 100 | Rejected | PASS |
| TC-ACD-12 | Insert duplicate student-subject grade | Rejected | PASS |
| TC-ACD-13 | Insert grade for nonexistent student | Rejected | PASS |
| TC-ACD-14 | Insert grade for nonexistent subject | Rejected | PASS |

## 4. SQLite Unit Tests

| ID | Test case | Expected result | Actual result |
|---|---|---|---|
| TC-SQL-01 | Open in-memory database | Connection successful | PASS |
| TC-SQL-02 | Create students table | Table created | PASS |
| TC-SQL-03 | Insert student record | Record inserted | PASS |
| TC-SQL-04 | Retrieve student record | Correct record retrieved | PASS |

## 5. Performance Tests

| ID | Test case | Expected result | Actual result |
|---|---|---|---|
| TC-PERF-01 | Student listing query | P95 below 3000 ms | PASS |
| TC-PERF-02 | Student search query | P95 below 3000 ms | PASS |
| TC-PERF-03 | Attendance summary query | P95 below 3000 ms | PASS |

## 6. Application Acceptance Tests

| ID | Test case | Test script | Verification |
|---|---|---|---|
| TC-ATT-01 | Record attendance | verify_attendance.sh | Success message |
| TC-ATT-02 | Calculate attendance percentage | verify_attendance.sh | Expected percentage |
| TC-ATT-03 | Update attendance | verify_attendance.sh | Updated percentage |
| TC-ATT-04 | Verify student access restrictions | verify_attendance.sh | Unauthorized operation denied |
| TC-RPT-01 | Generate full academic report | verify_reports.sh | Expected report sections |
| TC-RPT-02 | Generate grade sheet | verify_reports.sh | Grade sheet displayed |
| TC-RPT-03 | Generate attendance summary | verify_reports.sh | Attendance summary displayed |
| TC-RPT-04 | Verify role-based report access | verify_reports.sh | Role permissions enforced |

Note: Application acceptance test cases are included in the CI workflow. Their latest execution results must be taken from the corresponding GitHub Actions run.
