# Student Record System

A command-line application for managing student records, built with C++17
and SQLite. It lets authorized users manage student records, subjects,
grades, attendance, and academic reports with robust validation and
centralized role-based access control (RBAC).

---

## Features

- **User Authentication**: Secure PBKDF2-HMAC-SHA256 password hashing, brute-force lockout, and initial administrator bootstrap setup.
- **Centralized RBAC**: Role-based permissions enforcing least privilege across Admin, Faculty, Staff, and Student roles.
- **Student Record CRUD**: Create, view, search, update, and delete student records with validation.
- **Subject Management**: Create and list academic subjects with unique subject codes.
- **Grade Management**: Record and view course grades (0–100 scale) linked by foreign keys.
- **Attendance Tracking**: Record and update student attendance (Present, Absent, Late) with automatic percentage calculations.
- **Academic Reports & Summaries**: Generate comprehensive academic reports, standalone grade sheets, and per-subject attendance summaries.
- **Data Integrity**: Prepared statements, transaction safety, foreign key enforcement, and cascading cleanup on student deletion.

---

## Requirements

- **C++ Compiler**: GCC/G++ 9+ or Clang 10+ supporting C++17.
- **SQLite Development Library**:
  - Debian/Ubuntu: `libsqlite3-dev sqlite3`
  - macOS: `sqlite` (`brew install sqlite`)
  - Windows: MinGW-w64 with bundled `sqlite3.o` and `bcrypt`.
- **Build Tools**: `g++` or `clang++`.

---

## Build

The project consists of multiple modules that must be compiled together.

### Linux / macOS (Direct Compiler / CI Command)
```bash
g++ -std=c++17 -Wall -Wextra -Werror -O2 -Iinclude -Isrc \
    src/main.cpp src/auth.cpp src/grades.cpp src/password_hash.cpp \
    src/subjects.cpp src/attendance.cpp src/reports.cpp \
    -lsqlite3 -o student_system
```

### Windows (MinGW-w64)
```powershell
g++ -std=c++17 -Wall -Wextra -Werror -O2 -Iinclude -Isrc `
    src/main.cpp src/auth.cpp src/grades.cpp src/password_hash.cpp `
    src/subjects.cpp src/attendance.cpp src/reports.cpp `
    sqlite3.o -lbcrypt -o student_system.exe
```

---

## Run

1. Ensure the `data/` directory exists (or create it: `mkdir -p data`).
2. Run the executable:
   - Linux/macOS: `./student_system`
   - Windows: `.\student_system.exe`
3. The database file is created automatically at `data/student_records.db` on first launch.
4. **First-Time Administrator Setup**: If no user accounts exist in the database, the application prompts you to create the initial Administrator account (`setupFirstAdmin`). Once created, enter your credentials at the login prompt.

---

## Menu Overview

The interactive CLI presents options gated by the active user's permissions:

| Option | Menu Entry | Required Permission | Allowed Roles |
| :---: | :--- | :--- | :--- |
| **1** | Create student record | `Permission::CreateStudent` | Admin |
| **2** | View all students | `Permission::ViewStudents` | Admin, Faculty, Staff |
| **3** | Search student by ID | `Permission::SearchStudents` | Admin, Faculty, Staff |
| **4** | Update student record | `Permission::UpdateStudent` | Admin, Faculty |
| **5** | Delete student record | `Permission::DeleteStudent` | Admin, Faculty |
| **6** | Create user account | `Permission::ManageUsers` | Admin |
| **7** | Add subject | `Permission::ManageSubjects` | Admin |
| **8** | List subjects | `Permission::ManageSubjects` | Admin |
| **9** | Add grade | `Permission::AddGrades` | Admin, Faculty |
| **10** | View grades | `ViewAllGrades` / `ViewOwnGrades` | Admin, Faculty, Student (own) |
| **11** | Record attendance | `Permission::RecordAttendance` | Admin, Faculty |
| **12** | Update attendance | `Permission::UpdateAttendance` | Admin, Faculty |
| **13** | View attendance | `ViewAllAttendance` / `ViewOwnAttendance` | Admin, Faculty, Staff, Student (own) |
| **14** | Generate academic report | `Permission::GenerateAcademicReport` | Admin, Faculty |
| **15** | View grade sheet | `Permission::ViewGradeSheet` | Admin, Faculty, Staff, Student (own) |
| **16** | View attendance summary | `Permission::ViewAttendanceSummary` | Admin, Faculty, Staff, Student (own) |
| **0** | Exit | None (Public) | All |

---

## Roles and Permissions (RBAC Matrix)

| Permission | Admin | Faculty | Staff | Student |
| :--- | :---: | :---: | :---: | :---: |
| `CreateStudent` | ✓ | — | — | — |
| `ViewStudents` | ✓ | ✓ | ✓ | — |
| `SearchStudents` | ✓ | ✓ | ✓ | — |
| `UpdateStudent` | ✓ | ✓ | — | — |
| `DeleteStudent` | ✓ | ✓ | — | — |
| `ManageUsers` | ✓ | — | — | — |
| `ManageSubjects` | ✓ | — | — | — |
| `AddGrades` | ✓ | ✓ | — | — |
| `ViewAllGrades` | ✓ | ✓ | — | — |
| `ViewOwnGrades` | ✓ | ✓ | — | ✓ |
| `RecordAttendance` | ✓ | ✓ | — | — |
| `UpdateAttendance` | ✓ | ✓ | — | — |
| `ViewAllAttendance` | ✓ | ✓ | ✓ | — |
| `ViewOwnAttendance` | ✓ | ✓ | — | ✓ |
| `GenerateAcademicReport` | ✓ | ✓ | — | — |
| `ViewGradeSheet` | ✓ | ✓ | ✓ | ✓ |
| `ViewAttendanceSummary` | ✓ | ✓ | ✓ | ✓ |

*Note: Students are restricted to their own records (`student_id == username`) for grades, attendance, and reports.*

---

## Data Model

- **`students`**: Stores base student identity.
  - `student_id TEXT PRIMARY KEY`, `name TEXT NOT NULL`, `email TEXT`
- **`users`**: Manages credentials and role assignments.
  - `user_id INTEGER PRIMARY KEY AUTOINCREMENT`, `username TEXT NOT NULL UNIQUE`, `password_hash TEXT NOT NULL`, `role TEXT NOT NULL CHECK(role IN ('Admin','Faculty','Staff','Student'))`, `failed_attempts INTEGER NOT NULL DEFAULT 0`, `locked_until INTEGER NOT NULL DEFAULT 0`
- **`subjects`**: Course catalog.
  - `subject_id INTEGER PRIMARY KEY AUTOINCREMENT`, `subject_code TEXT NOT NULL UNIQUE`, `subject_name TEXT NOT NULL`
- **`grades`**: Student academic performance per subject.
  - `grade_id INTEGER PRIMARY KEY AUTOINCREMENT`, `student_id TEXT NOT NULL REFERENCES students(student_id)`, `subject_id INTEGER NOT NULL REFERENCES subjects(subject_id)`, `marks REAL NOT NULL CHECK(marks >= 0 AND marks <= 100)`, `UNIQUE(student_id, subject_id)`
- **`attendance`**: Daily session attendance tracking.
  - `attendance_id INTEGER PRIMARY KEY AUTOINCREMENT`, `student_id TEXT NOT NULL REFERENCES students(student_id) ON DELETE CASCADE`, `subject_id INTEGER NOT NULL REFERENCES subjects(subject_id) ON DELETE CASCADE`, `date TEXT NOT NULL`, `status TEXT NOT NULL CHECK(status IN ('Present', 'Absent', 'Late'))`, `UNIQUE(student_id, subject_id, date)`
  - Indexes: `idx_attendance_student`, `idx_attendance_subject`, `idx_attendance_student_subject`.

---

## CI/CD

Continuous integration is managed by GitHub Actions in [`.github/workflows/ci.yml`](.github/workflows/ci.yml):
- **Triggers**: Pushes to `main` and `feat/**`, and pull requests targeting `main`.
- **Matrix**: Runs parallel builds on `ubuntu-latest` and `macos-latest`.
- **Build & Artifacts**: Compiles the source files with `-Wall -Wextra -Werror -O2` and uploads the compiled `student_system` binary as an artifact (7 days retention).
- **Automated Verification**: Executes [`verify_attendance.sh`](verify_attendance.sh) and [`verify_reports.sh`](verify_reports.sh) in fresh, isolated environments.
- **Branch Policy**: `main` is protected; work happens on feature branches and merges via PR with green CI.

---

---

## Project Layout

```text
Student-Record-System/
├── .github/
│   └── workflows/
│       └── ci.yml               # GitHub Actions CI matrix workflow
├── data/
│   └── student_records.db       # SQLite database (created on first run)
├── include/
│   └── sqlite3.h                # SQLite C interface header
├── lib/
│   └── libsqlite3.a             # SQLite static/import library (Windows)
├── src/
│   ├── attendance.h/.cpp        # Attendance tracking and percentage calculations
│   ├── auth.h/.cpp              # Authentication, user setup, and account creation
│   ├── grades.h/.cpp            # Grade management and student grade queries
│   ├── main.cpp                 # Application entry point, CLI loop, RBAC dispatch
│   ├── password_hash.h/.cpp     # PBKDF2-HMAC-SHA256 password hashing
│   ├── rbac.h                   # Central RBAC permission enum and role checker
│   ├── reports.h/.cpp           # Academic reports, grade sheets, and summaries
│   └── subjects.h/.cpp          # Subject catalog management
├── tests/
│   ├── sqlite_test.cpp          # SQLite connection smoke test harness
│   └── subjects_grades_test.cpp # Subjects and grades test harness
├── verify_attendance.sh         # Attendance acceptance test script (TC-ATT-01/02/03)
├── verify_reports.sh            # Academic reports acceptance test script (TC-RPT-01/02)
└── README.md                    # Project documentation
```

---
