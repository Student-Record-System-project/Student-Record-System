# Student Record System

A command-line application for managing student records, built with C++17
and SQLite. It lets authorized users create, view, search, update, and
delete student records, with validation and role-based access control.

---

## What This Project Is

The Student Record System is a terminal-based tool for maintaining a
database of students. It stores data in a local SQLite file, so records
persist between runs. The system is designed around three principles:

- **Data integrity** — all writes use prepared statements and transactions,
  and required fields are validated before anything is saved.
- **Consistency** — deleting a student also removes their related grades
  and attendance records, so no orphaned data is left behind.
- **Access control** — only authorized roles (admin, teacher) can modify
  or delete records; other users can only read.

The project currently covers the core student record operations. Academic
reporting and full authentication are planned for later stages.

---

## Features

- Create student records with input validation
- View all students in a sorted list
- Search for a student by ID
- Update student information (admin / teacher only)
- Delete student records with confirmation (admin / teacher only)
- Automatic cleanup of related grades and attendance on deletion

---

## Prerequisites

You need a C++ compiler and the SQLite development library.

## How to Build and Run

**1. Create the data directory** (the program does not create it for you):

```bash
mkdir -p data
```

**2. Compile:**

```bash
g++ -std=c++17 -Wall -Wextra src/main.cpp -o student_system -lsqlite3
```

> The `-lsqlite3` flag must come after `src/main.cpp`, or the linker will
> fail with "undefined reference to sqlite3_open".

**3. Run:**

```bash
./student_system
```

The database file is created automatically at `data/student_records.db`
on first launch.

---

## How to Use

When you start the program, you'll see a menu:

```
=== Student Record System ===
1. Create student record
2. View all students
3. Search student by ID
5. Delete record
0. Exit
Choose:
```

- **1** — enter a student ID, name, and email to add a new record.
- **2** — list every student currently in the database.
- **3** — look up a single student by ID.
- **5** — delete a student. You must type `yes` to confirm.
- **0** — exit the program.

The database is saved to disk, so records remain available on the next run.

---

## Project Structure

```
Student-Record-System/
├── README.md
├── data/
│   └── student_records.db     # created on first run
└── src/
    └── main.cpp               # application source


```
