#include "password_hash.h"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <random>
#include <cstring>
using ULONG = uint32_t;
using UCHAR = uint8_t;
#endif

#include <array>
#include <vector>
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

#ifndef _WIN32
namespace sha256 {
inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t theta0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t theta1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

struct Context {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
};

inline void init(Context& ctx) {
    ctx.state[0] = 0x6a09e667; ctx.state[1] = 0xbb67ae85; ctx.state[2] = 0x3c6ef372; ctx.state[3] = 0xa54ff53a;
    ctx.state[4] = 0x510e527f; ctx.state[5] = 0x9b05688c; ctx.state[6] = 0x1f83d9ab; ctx.state[7] = 0x5be0cd19;
    ctx.count = 0;
}

inline void transform(Context& ctx, const uint8_t* data) {
    uint32_t W[64];
    for (int i = 0; i < 16; ++i) {
        W[i] = (static_cast<uint32_t>(data[i * 4]) << 24) |
               (static_cast<uint32_t>(data[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(data[i * 4 + 2]) << 8) |
               (static_cast<uint32_t>(data[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        W[i] = theta1(W[i - 2]) + W[i - 7] + theta0(W[i - 15]) + W[i - 16];
    }
    uint32_t a = ctx.state[0], b = ctx.state[1], c = ctx.state[2], d = ctx.state[3];
    uint32_t e = ctx.state[4], f = ctx.state[5], g = ctx.state[6], h = ctx.state[7];
    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + sig1(e) + choose(e, f, g) + K[i] + W[i];
        uint32_t t2 = sig0(a) + majority(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    ctx.state[0] += a; ctx.state[1] += b; ctx.state[2] += c; ctx.state[3] += d;
    ctx.state[4] += e; ctx.state[5] += f; ctx.state[6] += g; ctx.state[7] += h;
}

inline void update(Context& ctx, const uint8_t* data, size_t len) {
    size_t buffer_idx = (ctx.count / 8) % 64;
    ctx.count += len * 8;
    size_t part_len = 64 - buffer_idx;
    size_t i = 0;
    if (len >= part_len) {
        std::memcpy(&ctx.buffer[buffer_idx], data, part_len);
        transform(ctx, ctx.buffer);
        for (i = part_len; i + 63 < len; i += 64) {
            transform(ctx, &data[i]);
        }
        buffer_idx = 0;
    }
    std::memcpy(&ctx.buffer[buffer_idx], &data[i], len - i);
}

inline void final(Context& ctx, uint8_t digest[32]) {
    uint8_t final_count[8];
    for (int i = 0; i < 8; ++i) {
        final_count[i] = static_cast<uint8_t>((ctx.count >> ((7 - i) * 8)) & 0xff);
    }
    update(ctx, reinterpret_cast<const uint8_t*>("\x80"), 1);
    while ((ctx.count / 8) % 64 != 56) {
        update(ctx, reinterpret_cast<const uint8_t*>("\0"), 1);
    }
    update(ctx, final_count, 8);
    for (int i = 0; i < 8; ++i) {
        digest[i * 4]     = static_cast<uint8_t>((ctx.state[i] >> 24) & 0xff);
        digest[i * 4 + 1] = static_cast<uint8_t>((ctx.state[i] >> 16) & 0xff);
        digest[i * 4 + 2] = static_cast<uint8_t>((ctx.state[i] >> 8) & 0xff);
        digest[i * 4 + 3] = static_cast<uint8_t>(ctx.state[i] & 0xff);
    }
}

inline void hmac(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len, uint8_t out[32]) {
    uint8_t k[64] = {0};
    if (key_len > 64) {
        Context ctx;
        init(ctx);
        update(ctx, key, key_len);
        final(ctx, k);
    } else {
        std::memcpy(k, key, key_len);
    }
    uint8_t k_ipad[64], k_opad[64];
    for (int i = 0; i < 64; ++i) {
        k_ipad[i] = k[i] ^ 0x36;
        k_opad[i] = k[i] ^ 0x5c;
    }
    uint8_t ihash[32];
    Context ictx;
    init(ictx);
    update(ictx, k_ipad, 64);
    update(ictx, data, data_len);
    final(ictx, ihash);

    Context octx;
    init(octx);
    update(octx, k_opad, 64);
    update(octx, ihash, 32);
    final(octx, out);
}
} // namespace sha256
#endif

bool derive(const std::string& password, const UCHAR* salt,
            ULONG saltSize, ULONG iterations, UCHAR* output) {
#ifdef _WIN32
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
#else
    if (iterations == 0) return false;
    std::vector<uint8_t> salt_block(saltSize + 4);
    std::memcpy(salt_block.data(), salt, saltSize);
    salt_block[saltSize] = 0;
    salt_block[saltSize + 1] = 0;
    salt_block[saltSize + 2] = 0;
    salt_block[saltSize + 3] = 1;

    uint8_t u_prev[32];
    uint8_t u_xor[32];
    sha256::hmac(reinterpret_cast<const uint8_t*>(password.data()), password.size(),
                 salt_block.data(), salt_block.size(), u_prev);
    std::memcpy(u_xor, u_prev, 32);

    for (ULONG iter = 1; iter < iterations; ++iter) {
        uint8_t u_curr[32];
        sha256::hmac(reinterpret_cast<const uint8_t*>(password.data()), password.size(),
                     u_prev, 32, u_curr);
        for (size_t b = 0; b < 32; ++b) {
            u_xor[b] ^= u_curr[b];
        }
        std::memcpy(u_prev, u_curr, 32);
    }
    std::memcpy(output, u_xor, HASH_SIZE);
    return true;
#endif
}
}

bool hashPassword(const std::string& password, std::string& encoded) {
    if (password.empty() || password.size() > 1024) return false;

    std::array<UCHAR, SALT_SIZE> salt{};
    std::array<UCHAR, HASH_SIZE> hash{};

#ifdef _WIN32
    if (BCryptGenRandom(nullptr, salt.data(), SALT_SIZE,
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        return false;
#else
    std::random_device rd;
    for (size_t i = 0; i < SALT_SIZE; ++i) {
        salt[i] = static_cast<UCHAR>(rd() & 0xff);
    }
#endif

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
