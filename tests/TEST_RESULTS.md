
# Student Record System - Test Results

## 1. Testing Environment

| Component | Configuration |
|---|---|
| Operating System | Windows |
| Local Testing Terminal | MSYS2 UCRT64 |
| Compiler | GCC 16.2.0 |
| Programming Language | C++17 |
| Database | SQLite 3 |
| Compiler Flags | -Wall -Wextra -Werror |
| CI Platform | GitHub Actions |
| CI Operating Systems | Ubuntu, macOS and Windows |

## 2. Standalone Unit and Integration Test Results

Five standalone C++ test programs were compiled and executed successfully.

| Test ID | Test Program | Description | Result |
|---|---|---|---|
| TC-RBAC | rbac_test.cpp | Role permissions, case-insensitive roles and unknown role handling | PASS |
| TC-AUD | audit_test.cpp | Audit table initialization, event insertion and five-year retention | PASS |
| TC-ACD | academic_validation_test.cpp | Subject and grade initialization, constraints and academic data validation | PASS |
| TC-SQL | sqlite_test.cpp | In-memory SQLite connection, table creation, insertion and retrieval | PASS |
| TC-PERF | performance_test.cpp | Student listing, student search and attendance summary benchmarks | PASS |

### 2.1 RBAC Testing

Verified:
- Correct permission checks for four roles.
- Case-insensitive role name handling.
- Unknown roles are denied all permissions.

Result: All RBAC tests passed.

### 2.2 Audit Testing

Verified:
- Audit table initialization.
- Audit event insertion.
- Five-year audit log retention.

Result: All audit tests passed.

### 2.3 Academic Database Validation

Verified:
- Subject table initialization.
- Repeated subject table initialization.
- Existence of credits and semester columns.
- Student table creation.
- Grade table initialization.
- Insertion of test students and subjects.
- Rejection of duplicate subject codes.
- Acceptance of minimum marks (0).
- Acceptance of maximum marks (100).
- Rejection of negative marks.
- Rejection of marks above 100.
- Rejection of duplicate student-subject grades.
- Rejection of grades for nonexistent students.
- Rejection of grades for nonexistent subjects.

Result: All academic database validation tests passed.

### 2.4 SQLite Testing

Verified:
- Successful in-memory SQLite connection.
- Students table creation.
- Student record insertion.
- Student record retrieval.

Result: All SQLite tests passed.

## 3. Performance Testing Results

Performance testing was conducted using a synthetic dataset.

### Test Dataset

| Parameter | Value |
|---|---:|
| Students | 5,000 |
| Subjects | 10 |
| Attendance records | 250,000 |
| Warm-up runs | 5 |
| Measured runs | 100 |

### Benchmark Results

| Operation | Average (ms) | Median (ms) | P95 (ms) |
|---|---:|---:|---:|
| Student listing | 0.374689 | 0.3614 | 0.4605 |
| Student search | 0.002018 | 0.0020 | 0.0021 |
| Attendance summary | 0.01003 | 0.0100 | 0.0102 |

Performance acceptance target: P95 below 3000 ms.

Result: All three benchmark operations met the specified P95 target.

Note: These measurements represent SQLite query execution times, not complete application response times.

## 4. System and Acceptance Testing

The following application acceptance test scripts are integrated into the GitHub Actions workflow:

### 4.1 Attendance Acceptance Testing

Test script: `verify_attendance.sh`

The workflow executes the attendance acceptance tests against the compiled application.

### 4.2 Academic Reports Acceptance Testing

Test script: `verify_reports.sh`

The workflow executes the academic reports acceptance tests against the compiled application.

Result: Both acceptance test steps are included in the CI pipeline. Their execution status is verified through successful GitHub Actions runs.

## 5. Automated Testing Through CI

The testing workflow is integrated into GitHub Actions.

### Automated CI Activities

1. Checkout the repository.
2. Install required compiler and SQLite dependencies.
3. Compile the Student Record System application.
4. Compile five standalone C++ test programs.
5. Execute the standalone test programs.
6. Execute attendance acceptance tests.
7. Execute academic reports acceptance tests.
8. Build and execute coverage-instrumented tests.
9. Generate the code coverage report.
10. Upload the coverage report as a workflow artifact.

### CI Verification

The updated GitHub Actions workflow completed successfully after Pull Request #31 was merged into the main branch.

The Code Coverage (Ubuntu) job also completed successfully, including coverage report generation and artifact upload.

Result: CI workflow execution successful.

## 6. Code Coverage Results

Code coverage was measured using GCC coverage instrumentation and GCOVR.

### 6.1 Overall Coverage

| Metric | Covered | Total | Coverage |
|---|---:|---:|---:|
| Line coverage | 49 | 251 | 19.52% |
| Branch coverage | 40 | 610 | 6.56% |

### 6.2 Module-wise Coverage

| Source File | Line Coverage | Branch Coverage |
|---|---:|---:|
| src/audit.cpp | 89.47% | 50.00% |
| src/subjects.cpp | 16.78% | 6.96% |
| src/grades.cpp | 8.99% | 1.96% |

### 6.3 Coverage Interpretation

- The audit module achieved 89.47% line coverage.
- The subjects module achieved 16.78% line coverage.
- The grades module achieved 8.99% line coverage.

The coverage report currently includes only the audit, subjects and grades source modules exercised by the coverage tests.

It does not represent whole-project code coverage.

The coverage report was successfully generated as `coverage.xml` and uploaded as a GitHub Actions artifact.

## 7. Overall Testing Summary

| Testing Activity | Status |
|---|---|
| RBAC unit testing | PASS |
| Audit unit testing | PASS |
| Academic database validation | PASS |
| SQLite testing | PASS |
| Performance benchmarking | PASS |
| Attendance acceptance testing in CI | PASS |
| Academic reports acceptance testing in CI | PASS |
| Automated C++ test execution | PASS |
| Code coverage report generation | PASS |
| Coverage artifact upload | PASS |

## 8. Conclusion

The Student Record System underwent unit testing, integration testing, application acceptance testing and performance benchmarking.

Five standalone C++ test programs were successfully compiled and executed. The attendance and academic reports acceptance scripts were integrated into the CI pipeline.

GCC and GCOVR were used to generate a code coverage report for the audit, subjects and grades modules.

The updated GitHub Actions workflow completed successfully, and the coverage report was generated and uploaded.

The testing implementation and documentation were merged into the main branch through Pull Request #31.
