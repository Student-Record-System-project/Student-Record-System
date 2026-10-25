# Sprint 1 database coordination: Kojagori Chakraborty

## Verified current database
- Connection: `sqlite3_open("data/student_records.db", &db)` in `src/main.cpp`.
- One connection is passed to create, view, and search functions.
- Existing table: `students(student_id TEXT PRIMARY KEY, name TEXT NOT NULL, email TEXT)`.
- Inserts and searches use prepared statements.
- `src/sqlite_test.cpp` is a separate connection smoke test, not the application's shared connection.
- `data/` is ignored by Git and must exist before running the program.

## Proposed future tables (discussion only, NOT applied)
- `users`: user_id (primary key), username (unique), password_hash, role. Confirm role values and password-hashing approach with Himani.
- `subjects`: subject_id (primary key), subject_name, optional subject_code. Confirm columns with Himani.
- `grades`: grade_id (primary key), student_id (foreign key to students), subject_id (foreign key to subjects), grade. Confirm grade scale and uniqueness rules with Himani.
- Ibrahim should use the existing `students.student_id` as the record identifier for update/delete.

Do not add these tables until the team agrees on schema and migrations. Enable foreign key enforcement per SQLite connection (`PRAGMA foreign_keys = ON`) when introducing foreign keys. Preserve existing records.

## Implemented in this feature
- Whitespace trimming; non-empty/format/length checks for student ID and name; basic optional-email format checks.
- Search ID validation; checked SQL parameter binding.
- Distinguishes SQL read/search failures from no-results.
- Handles EOF in menu without an endless loop.

## Suggested regression tests
1. Create S1 / Alice / alice@example.com, then view and search S1.
2. Create S1 again: duplicate ID error.
3. Create with blank or whitespace ID/name: rejected.
4. Create with invalid email: rejected.
5. Search blank ID: rejected; search unknown valid ID: not found.
6. Enter letters at menu: rejected, menu remains usable.
7. Verify persisted data after restarting.

## Build
From project root, create `data/` first. On Linux with system SQLite development library:
`g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp -lsqlite3 -o student_records`
`./student_records`
On Windows use an installed compatible MinGW compiler and the supplied library, for example:
`g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp -Llib -lsqlite3 -o student_records.exe`
Run `student_records.exe` from the project root. Depending on the toolchain, `lib/sqlite3.dll` may need to be accessible on PATH or beside the executable. The supplied library/toolchain compatibility has not been tested here.
