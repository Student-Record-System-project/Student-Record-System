
#ifndef PASSWORD_INPUT_H
#define PASSWORD_INPUT_H

#include <iostream>
#include <string>

#ifdef _WIN32
#include <conio.h>
#include <cstdio>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

inline std::string readHiddenPassword() {
    std::string password;
    char ch;

    std::cout.flush();

#ifdef _WIN32

    // If stdin is redirected (for example, in CI tests),
    // read the password from the input stream instead of _getch().
    if (!_isatty(_fileno(stdin))) {
        std::getline(std::cin, password);
        return password;
    }

    // Interactive Windows terminal: hide password characters.
    while ((ch = static_cast<char>(_getch())) != '\r') {
        if (ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else if (ch >= 32 && ch <= 126) {
            password += ch;
            std::cout << '*';
        }
    }

#else

    // If terminal settings cannot be accessed, use normal input.
    termios oldSettings{}, newSettings{};

    if (tcgetattr(STDIN_FILENO, &oldSettings) != 0) {
        std::getline(std::cin, password);
        return password;
    }

    // Disable terminal echo while reading the password.
    newSettings = oldSettings;
    newSettings.c_lflag &= static_cast<tcflag_t>(~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    while (std::cin.get(ch) && ch != '\n') {
        if (ch == '\b' || ch == 127) {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else if (ch >= 32 && ch <= 126) {
            password += ch;
            std::cout << '*';
        }
    }

    // Restore terminal settings.
    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);

#endif

    std::cout << '\n';
    return password;
}

#endif
