#include "password_hash.h"

#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstdint>
#include <string>

namespace {
constexpr ULONG ITERATIONS = 200000;
constexpr ULONG SALT_SIZE = 16;
constexpr ULONG HASH_SIZE = 32;

std::string toHex(const UCHAR* data, size_t size) {
    static const char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 15]);
    }
    return out;
}

bool fromHex(const std::string& text, UCHAR* out, size_t size) {
    if (text.size() != size * 2) return false;
    auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < size; ++i) {
        int a = digit(text[i * 2]);
        int b = digit(text[i * 2 + 1]);
        if (a < 0 || b < 0) return false;
        out[i] = static_cast<UCHAR>((a << 4) | b);
    }
    return true;
}

bool derive(const std::string& password, const UCHAR* salt,
            ULONG saltSize, ULONG iterations, UCHAR* output) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
        BCRYPT_ALG_HANDLE_HMAC_FLAG);

    if (status < 0) return false;

    status = BCryptDeriveKeyPBKDF2(
        algorithm,
        reinterpret_cast<PUCHAR>(
            const_cast<char*>(password.data())),
        static_cast<ULONG>(password.size()),
        const_cast<PUCHAR>(salt), saltSize,
        iterations, output, HASH_SIZE, 0);

    BCryptCloseAlgorithmProvider(algorithm, 0);
    return status >= 0;
}
}

bool hashPassword(const std::string& password, std::string& encoded) {
    if (password.empty() || password.size() > 1024) return false;

    std::array<UCHAR, SALT_SIZE> salt{};
    std::array<UCHAR, HASH_SIZE> hash{};

    if (BCryptGenRandom(nullptr, salt.data(), SALT_SIZE,
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        return false;

    if (!derive(password, salt.data(), SALT_SIZE, ITERATIONS, hash.data()))
        return false;

    encoded = "pbkdf2-sha256$" + std::to_string(ITERATIONS) + "$" +
              toHex(salt.data(), salt.size()) + "$" +
              toHex(hash.data(), hash.size());
    return true;
}

bool verifyPassword(const std::string& password, const std::string& encoded) {
    const std::string prefix = "pbkdf2-sha256$";
    if (encoded.compare(0, prefix.size(), prefix) != 0) return false;

    size_t p1 = encoded.find('$', prefix.size());
    if (p1 == std::string::npos) return false;
    size_t p2 = encoded.find('$', p1 + 1);
    if (p2 == std::string::npos ||
        encoded.find('$', p2 + 1) != std::string::npos) return false;

    ULONG iterations = 0;
    try {
        size_t used = 0;
        unsigned long parsed = std::stoul(
            encoded.substr(prefix.size(), p1 - prefix.size()), &used);
        if (used != p1 - prefix.size() || parsed < 100000 ||
            parsed > 2000000) return false;
        iterations = static_cast<ULONG>(parsed);
    } catch (...) {
        return false;
    }

    std::array<UCHAR, SALT_SIZE> salt{};
    std::array<UCHAR, HASH_SIZE> expected{};
    std::array<UCHAR, HASH_SIZE> actual{};

    if (!fromHex(encoded.substr(p1 + 1, p2 - p1 - 1),
                 salt.data(), salt.size()) ||
        !fromHex(encoded.substr(p2 + 1), expected.data(), expected.size()))
        return false;

    if (!derive(password, salt.data(), SALT_SIZE, iterations, actual.data()))
        return false;

    UCHAR difference = 0;
    for (size_t i = 0; i < HASH_SIZE; ++i)
        difference |= actual[i] ^ expected[i];

    return difference == 0;
}
