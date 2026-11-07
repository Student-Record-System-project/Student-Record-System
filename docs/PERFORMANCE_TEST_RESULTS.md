# Student Record System — Performance Test Results

## Objective
Evaluate SQLite query performance against the Sprint 2 target of keeping
95% of dashboard/list requests below 3 seconds.

## Test environment
- Application: Student Record System
- Database: SQLite (in-memory test database)
- Compiler: g++ (C++17)
- Optimization: -O2
- Synthetic students: 5,000
- Subjects: 10
- Attendance records: 250,000
- Warm-up runs per query: 5
- Measured runs per query: 100

## Results

| Operation | Average (ms) | Median (ms) | P95 (ms) | Maximum (ms) |
|---|---:|---:|---:|---:|
| Student listing | 0.721089 | 0.6279 | 1.0429 | 1.1878 |
| Student search | 0.004297 | 0.0039 | 0.0060 | 0.0065 |
| Attendance summary | 0.025988 | 0.02375 | 0.0314 | 0.0411 |

## Acceptance threshold
Target: P95 latency below 3,000 ms.

All three measured database operations were below the threshold.

## Test execution

Compile:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -O2 -Iinclude -Isrc tests/performance_test.cpp -lsqlite3 -o performance_test.exe
./performance_test.exe

## Limitations
- Tests use synthetic data and an in-memory SQLite database.
- Measurements represent database query execution, not complete CLI response time.
- No web dashboard or HTTP endpoint was benchmarked.
- Results may vary across hardware and runtime environments.

## Conclusion
The measured student listing, student search, and attendance summary
queries satisfy the 3-second P95 database-query threshold in this test.
Full application response-time performance requires separate testing.
