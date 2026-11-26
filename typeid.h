/*
 * typeid.h - TypeID for C. Single header, zero dependencies, C99.
 * https://github.com/Sharkeeeeeee/typeid-c
 *
 * Implements the TypeID specification v0.3.0:
 *   https://github.com/jetify-com/typeid/tree/main/spec
 *
 * A TypeID is a type-safe, K-sortable, globally unique identifier built on
 * UUIDv7, for example:  user_01h455vb4pex5vsknk084sn02q
 *
 * USAGE
 *   In exactly ONE .c file, define TYPEID_IMPLEMENTATION before including:
 *
 *       #define TYPEID_IMPLEMENTATION
 *       #include "typeid.h"
 *
 *   Every other file just includes "typeid.h".
 *
 * OPTIONS (define before including, in the implementation file)
 *   TYPEID_STATIC         Make every function static to this translation unit.
 *   TYPEID_RANDOM(buf, n) Supply your own CSPRNG. Must fill `n` bytes of `buf`
 *                         and evaluate to 0 on success, non-zero on failure.
 *   TYPEID_NOW_MS()       Supply your own clock: Unix time in milliseconds,
 *                         as a uint64_t.
 *
 * LICENSE
 *   Apache License 2.0. See LICENSE at the end of this file.
 */
#ifndef TYPEID_H
#define TYPEID_H

#include <stddef.h>
#include <stdint.h>

#define TYPEID_VERSION "0.1.0"
#define TYPEID_SPEC_VERSION "0.3.0"

/* Maximum prefix length, in characters. */
#define TYPEID_PREFIX_MAX 63
/* Length of the base32 suffix, in characters. */
#define TYPEID_SUFFIX_LEN 26
/* Maximum length of a TypeID string, excluding the NUL terminator. */
#define TYPEID_MAX_LEN (TYPEID_PREFIX_MAX + 1 + TYPEID_SUFFIX_LEN)
/* Buffer size that always fits a TypeID string plus its NUL terminator. */
#define TYPEID_STR_SIZE (TYPEID_MAX_LEN + 1)
/* Buffer size for a canonical UUID string plus its NUL terminator. */
#define TYPEID_UUID_STR_SIZE 37

#if defined(TYPEID_STATIC) && (defined(__GNUC__) || defined(__clang__))
#define TYPEID_API static __attribute__((unused))
#elif defined(TYPEID_STATIC)
#define TYPEID_API static
#else
#define TYPEID_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct typeid_t {
    char prefix[TYPEID_PREFIX_MAX + 1]; /* NUL-terminated, may be empty */
    uint8_t uuid[16];                   /* big-endian UUID bytes */
} typeid_t;

typedef enum typeid_err {
    TYPEID_OK = 0,
    TYPEID_ERR_NULL,       /* a required pointer argument was NULL */
    TYPEID_ERR_PREFIX,     /* prefix is invalid (charset, length, underscores) */
    TYPEID_ERR_SUFFIX,     /* suffix has the wrong length or bad characters */
    TYPEID_ERR_OVERFLOW,   /* suffix encodes more than 128 bits */
    TYPEID_ERR_UUID,       /* malformed UUID string */
    TYPEID_ERR_BUFFER,     /* output buffer is too small */
    TYPEID_ERR_RANDOM,     /* the system random number generator failed */
    TYPEID_ERR_PREFIX_MISMATCH /* valid TypeID, but not the expected prefix */
} typeid_err;

/* Generate a new TypeID with a fresh UUIDv7. `prefix` may be "" or NULL. */
TYPEID_API typeid_err typeid_new(typeid_t *out, const char *prefix);

/* Build a TypeID from an existing 16-byte UUID (any version is accepted). */
TYPEID_API typeid_err typeid_from_uuid(typeid_t *out, const char *prefix,
                                       const uint8_t uuid[16]);

/* Build a TypeID from a UUID string such as
 * "01890a5d-ac96-774b-bcce-b302099a8057" (hyphens optional). */
TYPEID_API typeid_err typeid_from_uuid_str(typeid_t *out, const char *prefix,
                                           const char *uuid_str);

/* Parse a NUL-terminated TypeID string. */
TYPEID_API typeid_err typeid_parse(typeid_t *out, const char *str);

/* Parse a TypeID string of explicit length (need not be NUL-terminated). */
TYPEID_API typeid_err typeid_parse_n(typeid_t *out, const char *str, size_t len);

/* Write the canonical string form into `buf`. `buf_size` must be at least
 * strlen(prefix) + 28; TYPEID_STR_SIZE always fits. On success, `*out_len`
 * (if not NULL) receives the string length, excluding the NUL. */
TYPEID_API typeid_err typeid_to_string(const typeid_t *tid, char *buf,
                                       size_t buf_size, size_t *out_len);

/* Write the canonical UUID string (36 chars + NUL) into `buf`. */
TYPEID_API void typeid_uuid_string(const typeid_t *tid,
                                   char buf[TYPEID_UUID_STR_SIZE]);

/* Unix timestamp in milliseconds stored in the UUIDv7 (first 48 bits). */
TYPEID_API uint64_t typeid_timestamp_ms(const typeid_t *tid);

/* Compare two TypeIDs: by prefix, then by UUID. Returns <0, 0 or >0. */
TYPEID_API int typeid_compare(const typeid_t *a, const typeid_t *b);

/* Returns 1 if `prefix` (of length `len`) is a valid TypeID prefix. */
TYPEID_API int typeid_prefix_valid(const char *prefix, size_t len);

/* Low-level: encode 16 UUID bytes as the 26-char base32 suffix (+ NUL). */
TYPEID_API void typeid_encode_suffix(const uint8_t uuid[16],
                                     char out[TYPEID_SUFFIX_LEN + 1]);

/* Low-level: decode a 26-char base32 suffix into 16 UUID bytes. */
TYPEID_API typeid_err typeid_decode_suffix(const char *suffix, size_t len,
                                           uint8_t out[16]);

/* Low-level: parse a UUID string (with or without hyphens) into bytes. */
TYPEID_API typeid_err typeid_uuid_parse(const char *str, size_t len,
                                        uint8_t out[16]);

/* Write the 26-char base32 suffix (+ NUL) of `tid` into `buf`. */
TYPEID_API void typeid_suffix(const typeid_t *tid,
                              char buf[TYPEID_SUFFIX_LEN + 1]);

/* Parse `str` and require its prefix to equal `prefix` ("" or NULL means no
 * prefix). Returns TYPEID_ERR_PREFIX_MISMATCH if the prefix differs. */
TYPEID_API typeid_err typeid_parse_expect(typeid_t *out, const char *str,
                                          const char *prefix);

/* Returns 1 if the UUID is all zeros (the nil UUID), else 0. */
TYPEID_API int typeid_is_nil(const typeid_t *tid);

/* Returns 1 if both TypeIDs have the same prefix and UUID, else 0. */
TYPEID_API int typeid_equal(const typeid_t *a, const typeid_t *b);

/* ---- Monotonic generator ------------------------------------------------
 * typeid_new() produces random UUIDv7s: IDs created in the same millisecond
 * are not ordered relative to each other. A generator guarantees that every
 * ID it returns sorts strictly after the previous one, using the 12-bit
 * rand_a field as a counter (RFC 9562, section 6.2, method 1).
 *
 * A generator holds no locks: use one per thread, or protect a shared one
 * with your own mutex. Zero-initialize it, or call typeid_gen_init(). */
typedef struct typeid_gen_t {
    uint64_t last_ms; /* timestamp of the last ID */
    uint16_t counter; /* 12-bit counter within last_ms */
} typeid_gen_t;

TYPEID_API void typeid_gen_init(typeid_gen_t *gen);

/* Generate the next monotonic TypeID from `gen`. */
TYPEID_API typeid_err typeid_gen_next(typeid_gen_t *gen, typeid_t *out,
                                      const char *prefix);

/* Human-readable description of an error code. */
TYPEID_API const char *typeid_strerror(typeid_err err);

#ifdef __cplusplus
}
#endif

#endif /* TYPEID_H */

/* ======================================================================== */
/*                              IMPLEMENTATION                              */
/* ======================================================================== */

#ifdef TYPEID_IMPLEMENTATION
#ifndef TYPEID_IMPLEMENTATION_DONE
#define TYPEID_IMPLEMENTATION_DONE

#include <string.h>

/* ---- platform: random bytes -------------------------------------------- */

#ifndef TYPEID_RANDOM
#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#if defined(_MSC_VER)
#pragma comment(lib, "bcrypt.lib")
#endif
static int typeid__random(void *buf, size_t n) {
    return BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)n,
                           BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 0 : -1;
}
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || \
    defined(__NetBSD__) || defined(__DragonFly__)
#include <stdlib.h>
static int typeid__random(void *buf, size_t n) {
    arc4random_buf(buf, n);
    return 0;
}
#elif defined(__linux__) && defined(__GLIBC__) && \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))
#include <errno.h>
#include <sys/random.h>
static int typeid__random(void *buf, size_t n) {
    unsigned char *p = (unsigned char *)buf;
    while (n > 0) {
        ssize_t r = getrandom(p, n, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += r;
        n -= (size_t)r;
    }
    return 0;
}
#else
#include <stdio.h>
static int typeid__random(void *buf, size_t n) {
    FILE *f = fopen("/dev/urandom", "rb");
    size_t got;
    if (!f) return -1;
    got = fread(buf, 1, n, f);
    fclose(f);
    return got == n ? 0 : -1;
}
#endif
#define TYPEID_RANDOM(buf, n) typeid__random((buf), (n))
#endif /* TYPEID_RANDOM */

/* ---- platform: clock ---------------------------------------------------- */

#ifndef TYPEID_NOW_MS
#if defined(_WIN32)
#ifndef _WINDOWS_
#include <windows.h>
#endif
static uint64_t typeid__now_ms(void) {
    FILETIME ft;
    ULARGE_INTEGER t;
    GetSystemTimeAsFileTime(&ft);
    t.LowPart = ft.dwLowDateTime;
    t.HighPart = ft.dwHighDateTime;
    /* 100ns ticks since 1601-01-01 -> ms since 1970-01-01 */
    return (uint64_t)((t.QuadPart - 116444736000000000ULL) / 10000ULL);
}
#else
#include <time.h>
static uint64_t typeid__now_ms(void) {
#if defined(CLOCK_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
#elif defined(TIME_UTC)
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    /* Strict ISO C mode (e.g. -std=c99) hides clock_gettime and struct
     * timespec, but the function is always there on these systems. Declare
     * it with a layout-compatible struct. CLOCK_REALTIME is 0 on all of them. */
    struct typeid__timespec {
        time_t tv_sec;
        long tv_nsec;
    } ts;
    extern int clock_gettime(int, struct typeid__timespec *);
    if (clock_gettime(0, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
    return (uint64_t)time(NULL) * 1000u;
#else
    /* Unknown platform without POSIX or C11: only second precision. Define
     * TYPEID_NOW_MS() to supply a millisecond clock. */
    return (uint64_t)time(NULL) * 1000u;
#endif
}
#endif
#define TYPEID_NOW_MS() typeid__now_ms()
#endif /* TYPEID_NOW_MS */

/* ---- base32 ------------------------------------------------------------- */

static const char typeid__alphabet[32] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'j', 'k', 'm', 'n', 'p', 'q', 'r', 's', 't', 'v', 'w', 'x', 'y', 'z'};

/* Maps a byte to its 5-bit value, or 0xFF if it is not in the alphabet. */
static const uint8_t typeid__decode_table[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255,  10,  11,  12,  13,  14,  15,  16,  17, 255,  18,  19, 255,  20,  21, 255,
     22,  23,  24,  25,  26, 255,  27,  28,  29,  30,  31, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

static uint64_t typeid__load64(const uint8_t *p) {
    return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) |
           ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) |
           ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) |
           ((uint64_t)p[6] << 8) | (uint64_t)p[7];
}

static void typeid__store64(uint8_t *p, uint64_t v) {
    int i;
    for (i = 7; i >= 0; i--) {
        p[i] = (uint8_t)v;
        v >>= 8;
    }
}

static void typeid__encode(const uint8_t *uuid, char *out) {
    /* The UUID is a 128-bit big-endian number (hi:lo). Character i holds the
     * 5 bits starting at bit 125 - 5*i, counting from the least significant
     * bit; the first character only has 3 meaningful bits. */
    const uint64_t hi = typeid__load64(uuid), lo = typeid__load64(uuid + 8);
    int i;
    for (i = 0; i < TYPEID_SUFFIX_LEN; i++) {
        const int shift = 125 - 5 * i;
        uint64_t v;
        if (shift >= 64) v = hi >> (shift - 64);
        else if (shift > 59) v = (hi << (64 - shift)) | (lo >> shift);
        else v = lo >> shift;
        out[i] = typeid__alphabet[v & 31u];
    }
    out[TYPEID_SUFFIX_LEN] = '\0';
}

TYPEID_API void typeid_encode_suffix(const uint8_t uuid[16],
                                     char out[TYPEID_SUFFIX_LEN + 1]) {
    typeid__encode(uuid, out);
}

TYPEID_API typeid_err typeid_decode_suffix(const char *suffix, size_t len,
                                           uint8_t out[16]) {
    uint64_t hi = 0, lo = 0;
    unsigned bad = 0;
    int i;
    if (!suffix || !out) return TYPEID_ERR_NULL;
    if (len != TYPEID_SUFFIX_LEN) return TYPEID_ERR_SUFFIX;
    for (i = 0; i < TYPEID_SUFFIX_LEN; i++) {
        const uint8_t v = typeid__decode_table[(unsigned char)suffix[i]];
        bad |= v;
        hi = (hi << 5) | (lo >> 59);
        lo = (lo << 5) | (v & 31u);
    }
    if (bad & 0x80u) return TYPEID_ERR_SUFFIX;
    if (suffix[0] > '7') return TYPEID_ERR_OVERFLOW;
    typeid__store64(out, hi);
    typeid__store64(out + 8, lo);
    return TYPEID_OK;
}

/* ---- prefix ------------------------------------------------------------- */

TYPEID_API int typeid_prefix_valid(const char *prefix, size_t len) {
    size_t i;
    if (len == 0) return 1;
    if (!prefix || len > TYPEID_PREFIX_MAX) return 0;
    for (i = 0; i < len; i++) {
        char c = prefix[i];
        if (c == '_') {
            if (i == 0 || i == len - 1) return 0;
        } else if (c < 'a' || c > 'z') {
            return 0;
        }
    }
    return 1;
}

static typeid_err typeid__set_prefix(typeid_t *out, const char *prefix,
                                     size_t len) {
    if (!typeid_prefix_valid(prefix, len)) return TYPEID_ERR_PREFIX;
    if (len) memcpy(out->prefix, prefix, len);
    out->prefix[len] = '\0';
    return TYPEID_OK;
}

/* ---- UUID strings ------------------------------------------------------- */

static int typeid__hex(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

TYPEID_API typeid_err typeid_uuid_parse(const char *str, size_t len,
                                        uint8_t out[16]) {
    uint8_t tmp[16];
    size_t i, n = 0;
    int hyphens;
    if (!str || !out) return TYPEID_ERR_NULL;
    if (len == 36) hyphens = 1;
    else if (len == 32) hyphens = 0;
    else return TYPEID_ERR_UUID;
    for (i = 0; i < len;) {
        int hi, lo;
        if (hyphens && (i == 8 || i == 13 || i == 18 || i == 23)) {
            if (str[i] != '-') return TYPEID_ERR_UUID;
            i++;
            continue;
        }
        hi = typeid__hex((unsigned char)str[i]);
        lo = typeid__hex((unsigned char)str[i + 1]);
        if (hi < 0 || lo < 0) return TYPEID_ERR_UUID;
        tmp[n++] = (uint8_t)((hi << 4) | lo);
        i += 2;
    }
    memcpy(out, tmp, 16);
    return TYPEID_OK;
}

TYPEID_API void typeid_uuid_string(const typeid_t *tid,
                                   char buf[TYPEID_UUID_STR_SIZE]) {
    static const char hex[] = "0123456789abcdef";
    int i, j = 0;
    for (i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) buf[j++] = '-';
        buf[j++] = hex[tid->uuid[i] >> 4];
        buf[j++] = hex[tid->uuid[i] & 15];
    }
    buf[j] = '\0';
}

/* ---- public API --------------------------------------------------------- */

TYPEID_API typeid_err typeid_from_uuid(typeid_t *out, const char *prefix,
                                       const uint8_t uuid[16]) {
    typeid_err e;
    if (!out || !uuid) return TYPEID_ERR_NULL;
    e = typeid__set_prefix(out, prefix ? prefix : "",
                           prefix ? strlen(prefix) : 0);
    if (e != TYPEID_OK) return e;
    memcpy(out->uuid, uuid, 16);
    return TYPEID_OK;
}

TYPEID_API typeid_err typeid_from_uuid_str(typeid_t *out, const char *prefix,
                                           const char *uuid_str) {
    uint8_t u[16];
    typeid_err e;
    if (!out || !uuid_str) return TYPEID_ERR_NULL;
    e = typeid_uuid_parse(uuid_str, strlen(uuid_str), u);
    if (e != TYPEID_OK) return e;
    return typeid_from_uuid(out, prefix, u);
}

TYPEID_API typeid_err typeid_new(typeid_t *out, const char *prefix) {
    uint8_t u[16];
    uint64_t ms;
    typeid_err e;
    if (!out) return TYPEID_ERR_NULL;
    if (TYPEID_RANDOM(u + 6, 10) != 0) return TYPEID_ERR_RANDOM;
    ms = TYPEID_NOW_MS();
    u[0] = (uint8_t)(ms >> 40);
    u[1] = (uint8_t)(ms >> 32);
    u[2] = (uint8_t)(ms >> 24);
    u[3] = (uint8_t)(ms >> 16);
    u[4] = (uint8_t)(ms >> 8);
    u[5] = (uint8_t)ms;
    u[6] = (uint8_t)((u[6] & 0x0F) | 0x70); /* version 7 */
    u[8] = (uint8_t)((u[8] & 0x3F) | 0x80); /* RFC 9562 variant */
    e = typeid_from_uuid(out, prefix, u);
    return e;
}

TYPEID_API typeid_err typeid_parse_n(typeid_t *out, const char *str,
                                     size_t len) {
    size_t sep;
    const char *suffix;
    size_t suffix_len, prefix_len;
    typeid_t tmp;
    typeid_err e;
    if (!out || !str) return TYPEID_ERR_NULL;
    if (len > TYPEID_MAX_LEN) {
        /* Report the most useful error for over-long input. */
        return len > TYPEID_SUFFIX_LEN + 1 &&
                       str[len - TYPEID_SUFFIX_LEN - 1] == '_'
                   ? TYPEID_ERR_PREFIX
                   : TYPEID_ERR_SUFFIX;
    }

    /* The separator is the last underscore; the suffix never contains one. */
    sep = len;
    while (sep > 0 && str[sep - 1] != '_') sep--;
    if (sep == 0) {
        prefix_len = 0;
        suffix = str;
        suffix_len = len;
    } else {
        prefix_len = sep - 1;
        suffix = str + sep;
        suffix_len = len - sep;
        /* A separator requires a non-empty prefix. */
        if (prefix_len == 0) return TYPEID_ERR_PREFIX;
    }

    e = typeid__set_prefix(&tmp, str, prefix_len);
    if (e != TYPEID_OK) return e;
    e = typeid_decode_suffix(suffix, suffix_len, tmp.uuid);
    if (e != TYPEID_OK) return e;
    *out = tmp;
    return TYPEID_OK;
}

TYPEID_API typeid_err typeid_parse(typeid_t *out, const char *str) {
    if (!out || !str) return TYPEID_ERR_NULL;
    return typeid_parse_n(out, str, strlen(str));
}

TYPEID_API typeid_err typeid_to_string(const typeid_t *tid, char *buf,
                                       size_t buf_size, size_t *out_len) {
    size_t plen, total;
    if (!tid || !buf) return TYPEID_ERR_NULL;
    plen = strlen(tid->prefix);
    total = plen + (plen ? 1 : 0) + TYPEID_SUFFIX_LEN;
    if (buf_size < total + 1) return TYPEID_ERR_BUFFER;
    if (plen) {
        memcpy(buf, tid->prefix, plen);
        buf[plen] = '_';
        typeid__encode(tid->uuid, buf + plen + 1);
    } else {
        typeid__encode(tid->uuid, buf);
    }
    if (out_len) *out_len = total;
    return TYPEID_OK;
}

TYPEID_API uint64_t typeid_timestamp_ms(const typeid_t *tid) {
    uint64_t ms = 0;
    int i;
    for (i = 0; i < 6; i++) ms = (ms << 8) | tid->uuid[i];
    return ms;
}

TYPEID_API int typeid_compare(const typeid_t *a, const typeid_t *b) {
    int c = strcmp(a->prefix, b->prefix);
    if (c) return c;
    return memcmp(a->uuid, b->uuid, 16);
}

TYPEID_API void typeid_suffix(const typeid_t *tid,
                              char buf[TYPEID_SUFFIX_LEN + 1]) {
    typeid__encode(tid->uuid, buf);
}

TYPEID_API typeid_err typeid_parse_expect(typeid_t *out, const char *str,
                                          const char *prefix) {
    typeid_t tmp;
    typeid_err e;
    if (!out || !str) return TYPEID_ERR_NULL;
    e = typeid_parse(&tmp, str);
    if (e != TYPEID_OK) return e;
    if (strcmp(tmp.prefix, prefix ? prefix : "") != 0)
        return TYPEID_ERR_PREFIX_MISMATCH;
    *out = tmp;
    return TYPEID_OK;
}

TYPEID_API int typeid_is_nil(const typeid_t *tid) {
    int i;
    for (i = 0; i < 16; i++)
        if (tid->uuid[i]) return 0;
    return 1;
}

TYPEID_API int typeid_equal(const typeid_t *a, const typeid_t *b) {
    return typeid_compare(a, b) == 0;
}

TYPEID_API void typeid_gen_init(typeid_gen_t *gen) {
    gen->last_ms = 0;
    gen->counter = 0;
}

TYPEID_API typeid_err typeid_gen_next(typeid_gen_t *gen, typeid_t *out,
                                      const char *prefix) {
    uint8_t u[16];
    uint64_t ms;
    if (!gen || !out) return TYPEID_ERR_NULL;
    if (!typeid_prefix_valid(prefix, prefix ? strlen(prefix) : 0))
        return TYPEID_ERR_PREFIX;
    if (TYPEID_RANDOM(u + 6, 10) != 0) return TYPEID_ERR_RANDOM;
    ms = TYPEID_NOW_MS();
    if (ms > gen->last_ms) {
        /* New millisecond: seed the counter randomly in its lower half so
         * there is plenty of room to increment. */
        gen->last_ms = ms;
        gen->counter = (uint16_t)(((u[6] << 8) | u[7]) & 0x07FF);
    } else if (gen->counter < 0x0FFF) {
        /* Same millisecond, or the clock went backwards: keep last_ms. */
        gen->counter++;
    } else {
        /* Counter exhausted: borrow the next millisecond. */
        gen->last_ms++;
        gen->counter = 0;
    }
    ms = gen->last_ms;
    u[0] = (uint8_t)(ms >> 40);
    u[1] = (uint8_t)(ms >> 32);
    u[2] = (uint8_t)(ms >> 24);
    u[3] = (uint8_t)(ms >> 16);
    u[4] = (uint8_t)(ms >> 8);
    u[5] = (uint8_t)ms;
    u[6] = (uint8_t)(0x70 | (gen->counter >> 8)); /* version 7 + counter */
    u[7] = (uint8_t)gen->counter;
    u[8] = (uint8_t)((u[8] & 0x3F) | 0x80); /* RFC 9562 variant */
    return typeid_from_uuid(out, prefix, u);
}

TYPEID_API const char *typeid_strerror(typeid_err err) {
    switch (err) {
    case TYPEID_OK: return "ok";
    case TYPEID_ERR_NULL: return "null argument";
    case TYPEID_ERR_PREFIX:
        return "invalid prefix: must be at most 63 chars of [a-z_], "
               "starting and ending with a letter";
    case TYPEID_ERR_SUFFIX:
        return "invalid suffix: must be 26 chars of "
               "0123456789abcdefghjkmnpqrstvwxyz";
    case TYPEID_ERR_OVERFLOW:
        return "invalid suffix: value exceeds 128 bits (first char must be <= '7')";
    case TYPEID_ERR_UUID: return "invalid UUID string";
    case TYPEID_ERR_BUFFER: return "output buffer too small";
    case TYPEID_ERR_RANDOM: return "random number generator failed";
    case TYPEID_ERR_PREFIX_MISMATCH: return "TypeID has a different prefix than expected";
    }
    return "unknown error";
}

#endif /* TYPEID_IMPLEMENTATION_DONE */
#endif /* TYPEID_IMPLEMENTATION */

/*
 * LICENSE
 *
 * Copyright 2026 Sharkeeeeeee
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * The TypeID specification and test vectors are Copyright Jetify Inc.,
 * licensed under Apache 2.0: https://github.com/jetify-com/typeid
 */
