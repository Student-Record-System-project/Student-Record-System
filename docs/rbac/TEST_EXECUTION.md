# Test Execution Instructions

The RBAC test source is located at tests/rbac_test.cpp.

A C++17 compiler is required.

Example using GCC:

g++ -std=c++17 -Wall -Wextra -Werror -Isrc tests/rbac_test.cpp -o rbac_test

Run the generated executable:

./rbac_test

Do not report a passing result unless the executable was run successfully.
