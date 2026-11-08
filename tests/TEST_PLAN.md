
# Student Record System - Test Plan

## 1. Introduction

This document defines the testing approach for the Student Record System, a C++ application that uses SQLite for persistent student academic records.

The objective is to verify the correctness of individual modules, interactions between modules, complete application workflows, and selected non-functional requirements.

## 2. Testing Objectives

- Verify that individual modules perform their intended operations.
- Validate role-based access control and audit logging.
- Verify subject and grade database operations.
- Test attendance and academic report workflows through the application.
- Evaluate database query performance using a synthetic dataset.
- Verify that invalid inputs and database operations are handled correctly.
- Automate repeatable tests through GitHub Actions.

## 3. Scope of Testing

### 3.1 Unit Testing

Individual components are tested independently.

Test programs:

- `rbac_test.cpp`
- `audit_test.cpp`
- `sqlite_test.cpp`

### 3.2 Integration Testing

Related modules are tested together to verify their interactions.

Test program:

- `academic_validation_test.cpp`

This test exercises subject, grade and audit module implementations with SQLite.

### 3.3 System Testing

The complete application is tested through automated acceptance scripts.

Test scripts:

- `verify_attendance.sh`
- `verify_reports.sh`

These scripts execute the compiled application and verify expected application behaviour.

### 3.4 Non-Functional Testing

Performance testing is conducted using:

- `performance_test.cpp`

The benchmark measures student listing, student search and attendance summary query execution times.

## 4. Test Environment

| Component | Environment |
|---|---|
| Programming language | C++17 |
| Database | SQLite |
| Compiler | GCC |
| Build system | g++ command-line compilation |
| Local testing environment | MSYS2 UCRT64 |
| CI platform | GitHub Actions |
| Operating systems in CI | Ubuntu, macOS and Windows |

## 5. Test Data

- In-memory SQLite databases are used by isolated database tests.
- Synthetic student and attendance records are used for performance benchmarking.
- Temporary application databases are used by acceptance scripts.
- Test accounts and academic records are created during acceptance testing.

## 6. Test Execution Strategy

1. Compile the application.
2. Compile standalone C++ test programs.
3. Execute the standalone tests.
4. Execute attendance acceptance tests.
5. Execute academic reports acceptance tests.
6. Run performance benchmarks.
7. Generate code coverage reports through GCOV and GCOVR.
8. Record actual results and investigate failures.

## 7. Entry Criteria

- Source code is available.
- Required compiler and SQLite dependencies are installed.
- Test programs and acceptance scripts are available.
- The Git branch contains the intended testing changes.

## 8. Exit Criteria

- All required test programs compile successfully.
- Automated test assertions pass.
- Existing application acceptance tests pass.
- Performance measurements are recorded.
- Coverage report is generated successfully.
- Test results and known limitations are documented.

## 9. Limitations

- Performance benchmark measurements represent SQLite query timings, not complete application response times.
- The interactive `subjects_grades_test.cpp` utility is intended for manual testing and is not included in unattended CI execution.
- Initial code coverage is limited to the source modules instrumented and exercised by the coverage tests.
- Passing individual module tests does not establish complete end-to-end integration coverage.
