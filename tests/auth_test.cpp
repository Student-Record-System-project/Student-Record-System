
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>

#include "sqlite3.h"
#include "auth.h"
#include "password_hash.h"

int passed = 0;
int failed = 0;

void check(bool condition, const char* description) {
    if (condition) {
        std::cout << "PASS: " << description << '\n';
        ++passed;
    } else {
        std::cerr << "FAIL: " << description << '\n';
        ++failed;
    }
}

bool queryInt(sqlite3* db, const char* sql, int& value) {
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    bool success = sqlite3_step(stmt) == SQLITE_ROW;

    if (success)
        value = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return success;
}

bool loginWithInput(
    sqlite3* db,
    const std::string& input,
    std::string& username,
    std::string& role
) {
    std::istringstream simulatedInput(input);
    std::streambuf* originalInput = std::cin.rdbuf(simulatedInput.rdbuf());

    bool result = loginUser(db, username, role);

    std::cin.rdbuf(originalInput);
    return result;
}

int main() {
    std::string hash1;
    std::string hash2;

    check(hashPassword("StrongPassword123", hash1),
          "Hash valid password");

    check(hash1.find("pbkdf2-sha256$") == 0,
          "Verify PBKDF2-SHA256 encoded format");

    check(verifyPassword("StrongPassword123", hash1),
          "Verify correct password");

    check(!verifyPassword("WrongPassword123", hash1),
          "Reject incorrect password");

    check(hashPassword("StrongPassword123", hash2),
          "Hash same password again");

    check(hash1 != hash2,
          "Generate different salted hashes");

    check(!hashPassword("", hash1),
          "Reject empty password");

    check(!hashPassword(std::string(1025, 'a'), hash1),
          "Reject oversized password");

    check(!verifyPassword("password", "invalid-format"),
          "Reject malformed encoded hash");

    check(!verifyPassword("password",
          "pbkdf2-sha256$99999$00112233445566778899aabbccddeeff$"
          "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff"),
          "Reject iteration count below minimum");

    sqlite3* db = nullptr;

    check(sqlite3_open(":memory:", &db) == SQLITE_OK,
          "Open in-memory authentication database");

    if (!db) {
        std::cerr << "Cannot continue without database.\n";
        return 1;
    }

    check(initializeAuth(db),
          "Initialize authentication and audit tables");

    check(createUser(db, "test_admin", "StrongPassword123", "Admin"),
          "Create valid administrator");

    check(!createUser(db, "test_admin", "StrongPassword123", "Admin"),
          "Reject duplicate username");

    check(!createUser(db, "x", "StrongPassword123", "Admin"),
          "Reject short username");

    check(!createUser(db, "invalid user", "StrongPassword123", "Admin"),
          "Reject username containing spaces");

    check(!createUser(db, "test_user", "short", "Student"),
          "Reject short account password");

    check(!createUser(db, "test_user", "StrongPassword123", "InvalidRole"),
          "Reject invalid user role");

    std::string username;
    std::string role;

    check(loginWithInput(db,
          "test_admin\nStrongPassword123\n", username, role),
          "Login with valid credentials");

    check(role == "Admin",
          "Return correct authenticated role");

    int failures = -1;

    check(queryInt(db,
          "SELECT failed_attempts FROM users WHERE username='test_admin';",
          failures) && failures == 0,
          "Reset failed attempts after successful login");

    check(!loginWithInput(db,
          "test_admin\nWrongPassword123\n", username, role),
          "Reject incorrect login password");

    check(queryInt(db,
          "SELECT failed_attempts FROM users WHERE username='test_admin';",
          failures) && failures == 1,
          "Increment failed login counter");

    check(!loginWithInput(db,
          "unknown_user\nWrongPassword123\n", username, role),
          "Reject unknown username");

    check(queryInt(db,
          "SELECT COUNT(*) FROM audit_logs WHERE action='LOGIN_FAILED';",
          failures) && failures >= 2,
          "Record failed login audit events");

    // Complete the remaining failed attempts.
    for (int i = 0; i < 4; ++i) {
        loginWithInput(db,
            "test_admin\nWrongPassword123\n", username, role);
    }

    sqlite3_stmt* lockStmt = nullptr;
    bool lockVerified = false;

    if (sqlite3_prepare_v2(db,
        "SELECT failed_attempts, locked_until FROM users "
        "WHERE username='test_admin';",
        -1, &lockStmt, nullptr) == SQLITE_OK) {

        if (sqlite3_step(lockStmt) == SQLITE_ROW) {
            int attempts = sqlite3_column_int(lockStmt, 0);
            sqlite3_int64 lockedUntil = sqlite3_column_int64(lockStmt, 1);

            lockVerified = attempts == 0 &&
                lockedUntil > static_cast<sqlite3_int64>(time(nullptr));
        }

        sqlite3_finalize(lockStmt);
    }

    check(lockVerified,
          "Lock account after five failed attempts for 15 minutes");

    check(!loginWithInput(db,
          "test_admin\nStrongPassword123\n", username, role),
          "Block login during account lockout");

    check(queryInt(db,
          "SELECT COUNT(*) FROM audit_logs WHERE action='ACCOUNT_LOCKED';",
          failures) && failures == 1,
          "Record account lockout audit event");

    std::cout << "\n============================\n";
    std::cout << "Passed: " << passed << '\n';
    std::cout << "Failed: " << failed << '\n';
    std::cout << "============================\n";

    sqlite3_close(db);

    return failed == 0 ? 0 : 1;
}
