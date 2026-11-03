#include <iostream>
#include <limits>
#include "sqlite3.h"
#include "subjects.h"
#include "grades.h"

using namespace std;

int main() {
    sqlite3* db = nullptr;

    if (sqlite3_open("data/student_records.db", &db) != SQLITE_OK) {
        cerr << "Could not open database: "
             << (db ? sqlite3_errmsg(db) : "unknown error") << '\n';
        if (db) sqlite3_close(db);
        return 1;
    }

    if (!initializeSubjects(db) || !initializeGrades(db)) {
        cerr << "Could not initialize subject/grade tables.\n";
        sqlite3_close(db);
        return 1;
    }

    int choice;
    do {
        cout << "\n===== SUBJECTS AND GRADES TEST =====\n"
             << "1. Add subject\n"
             << "2. List subjects\n"
             << "3. Add grade\n"
             << "4. List grades for a student\n"
             << "0. Exit\n"
             << "Choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Enter a valid menu number.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1: addSubject(db); break;
            case 2: listSubjects(db); break;
            case 3: addGrade(db); break;
            case 4: listGradesForStudent(db); break;
            case 0: cout << "Test program closed.\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (choice != 0);

    sqlite3_close(db);
    return 0;
}
