#ifndef PASSWORD_HASH_H
#define PASSWORD_HASH_H

#include <string>

bool hashPassword(const std::string& password, std::string& encoded);
bool verifyPassword(const std::string& password, const std::string& encoded);

#endif
