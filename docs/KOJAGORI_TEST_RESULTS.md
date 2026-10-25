# Kojagori Sprint 1 test evidence

Test environment: Linux, g++ C++17, system SQLite library, fresh ignored `data/student_records.db`.

- [x] Compilation with `-Wall -Wextra` succeeded.
- [x] Create student S1 / Alice / alice@example.com succeeded.
- [x] View all students displayed S1.
- [x] Search for S1 returned Alice and email.

Run and record remaining manual checks locally before opening PR:
- [ ] Duplicate ID rejected
- [ ] Whitespace-only ID/name rejected
- [ ] Invalid email rejected
- [ ] Empty search ID rejected
- [ ] Unknown student search handled
- [ ] Invalid menu input handled
- [ ] Restart and verify persisted record
- [ ] Windows build and run in teammate's environment
