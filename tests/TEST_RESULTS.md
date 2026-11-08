
# Student Record System - Test Results

## 1. Testing Environment

- Operating system: Windows
- Terminal: MSYS2 UCRT64
- Compiler: GCC 16.2.0
- Database: SQLite 3
- Language standard: C++17
- Compiler flags: -Wall -Wextra -Werror

## 2. Standalone Test Execution Results

| Test program | Compilation | Execution | Result |
|---|---|---|---|
| rbac_test.cpp | Successful | All permission checks passed | PASS |
| audit_test.cpp | Successful | All audit checks passed | PASS |
| academic_validation_test.cpp | Successful | All validation checks passed | PASS |
| performance_test.cpp | Successful | All three benchmarks completed | PASS |
| sqlite_test.cpp | Successful | All SQLite checks passed | PASS |

All five standalone C++ test programs also compiled successfully together using the strict compiler flags.

## 3. Performance Benchmark Results

Test dataset:

- Students: 5,000
- Subjects: 10
- Attendance records: 250,000
- Warm-up runs: 5
- Measured runs: 100

| Operation | Average (ms) | Median (ms) | P95 (ms) |
|---|---:|---:|---:|
| Student listing | 0.374689 | 0.3614 | 0.4605 |
| Student search | 0.002018 | 0.0020 | 0.0021 |
| Attendance summary | 0.01003 | 0.0100 | 0.0102 |

Performance acceptance target: P95 below 3000 ms.

Result: All three measured operations met the specified benchmark target.

Note: These measurements represent SQLite query execution times, not full application response times.

## 4. Application Acceptance Testing

The following scripts are configured in the GitHub Actions workflow:

- verify_attendance.sh
- verify_reports.sh

Their results must be recorded from the latest successful GitHub Actions execution. Previous workflow inclusion does not by itself establish the result of the updated workflow.

## 5. Code Coverage

A separate GitHub Actions coverage job has been configured using GCC coverage instrumentation and GCOVR.

Initial coverage scope:

- src/audit.cpp
- src/subjects.cpp
- src/grades.cpp

The coverage report is generated as coverage.xml and uploaded as a GitHub Actions artifact.

Coverage percentage: Pending GitHub Actions execution.

## 6. Overall Result

- Five standalone C++ tests: Successfully compiled and executed locally.
- Performance benchmark: All three measured queries met the P95 target.
- Attendance and reports acceptance tests: Configured in CI; latest execution pending.
- Code coverage: Configured; report pending CI execution.

Testing is not declared fully complete until the updated GitHub Actions workflow succeeds and the coverage artifact is generated.
