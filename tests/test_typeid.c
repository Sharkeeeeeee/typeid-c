/* Test suite for typeid.h. Runs the official spec vectors plus extra cases. */
#define TYPEID_IMPLEMENTATION
#include "../typeid.h"
#include "vectors.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...)                                              \
    do {                                                              \
        checks++;                                                     \
        if (!(cond)) {                                                \
            failures++;                                               \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);      \
            fprintf(stderr, __VA_ARGS__);                             \
            fprintf(stderr, "\n");                                    \
        }                                                             \
    } while (0)

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

static void test_valid_vectors(void) {
    size_t i;
    for (i = 0; i < COUNT(VALID); i++) {
        const valid_vector *v = &VALID[i];
        typeid_t tid, built;
        char str[TYPEID_STR_SIZE], uuid[TYPEID_UUID_STR_SIZE];
        typeid_err e;

        /* decode: string -> (prefix, uuid) */
        e = typeid_parse(&tid, v->str);
        CHECK(e == TYPEID_OK, "[%s] parse failed: %s", v->name, typeid_strerror(e));
        if (e != TYPEID_OK) continue;
        CHECK(strcmp(tid.prefix, v->prefix) == 0, "[%s] prefix %s != %s", v->name,
              tid.prefix, v->prefix);
        typeid_uuid_string(&tid, uuid);
        CHECK(strcmp(uuid, v->uuid) == 0, "[%s] uuid %s != %s", v->name, uuid, v->uuid);

        /* encode: (prefix, uuid) -> string */
        e = typeid_from_uuid_str(&built, v->prefix, v->uuid);
        CHECK(e == TYPEID_OK, "[%s] from_uuid_str failed: %s", v->name, typeid_strerror(e));
        e = typeid_to_string(&built, str, sizeof str, NULL);
        CHECK(e == TYPEID_OK, "[%s] to_string failed", v->name);
        CHECK(strcmp(str, v->str) == 0, "[%s] encoded %s != %s", v->name, str, v->str);
        CHECK(typeid_compare(&tid, &built) == 0, "[%s] compare != 0", v->name);
    }
}

static void test_invalid_vectors(void) {
    size_t i;
    for (i = 0; i < COUNT(INVALID); i++) {
        const invalid_vector *v = &INVALID[i];
        typeid_t tid;
        typeid_err e = typeid_parse(&tid, v->str);
        CHECK(e != TYPEID_OK, "[%s] accepted invalid typeid \"%s\" (%s)", v->name,
              v->str, v->description);
    }
}

static void test_new(void) {
    typeid_t a, b, parsed;
    char s[TYPEID_STR_SIZE];
    size_t len = 0;
    int i;

    CHECK(typeid_new(&a, "user") == TYPEID_OK, "typeid_new failed");
    CHECK(strcmp(a.prefix, "user") == 0, "prefix not set");
    CHECK((a.uuid[6] >> 4) == 7, "not a UUIDv7 (version nibble %x)", a.uuid[6] >> 4);
    CHECK((a.uuid[8] & 0xC0) == 0x80, "wrong UUID variant");
    CHECK(typeid_timestamp_ms(&a) > 1700000000000ULL, "timestamp looks wrong");

    CHECK(typeid_to_string(&a, s, sizeof s, &len) == TYPEID_OK, "to_string");
    CHECK(len == 4 + 1 + 26 && strlen(s) == len, "length %u", (unsigned)len);
    CHECK(typeid_parse(&parsed, s) == TYPEID_OK, "roundtrip parse");
    CHECK(typeid_compare(&a, &parsed) == 0, "roundtrip mismatch");

    CHECK(typeid_new(&b, NULL) == TYPEID_OK, "new with NULL prefix");
    CHECK(b.prefix[0] == '\0', "NULL prefix should be empty");
    CHECK(typeid_new(&b, "") == TYPEID_OK, "new with empty prefix");
    CHECK(typeid_to_string(&b, s, sizeof s, &len) == TYPEID_OK && len == 26, "no-prefix len");

    /* uniqueness smoke test */
    for (i = 0; i < 1000; i++) {
        CHECK(typeid_new(&b, "user") == TYPEID_OK, "new");
        CHECK(memcmp(a.uuid, b.uuid, 16) != 0, "duplicate uuid");
    }

    CHECK(typeid_new(&a, "User") == TYPEID_ERR_PREFIX, "uppercase prefix accepted");
    CHECK(typeid_new(&a, "_user") == TYPEID_ERR_PREFIX, "leading _ accepted");
    CHECK(typeid_new(&a, "user_") == TYPEID_ERR_PREFIX, "trailing _ accepted");
    CHECK(typeid_new(NULL, "user") == TYPEID_ERR_NULL, "NULL out accepted");
}

static void test_prefix_rules(void) {
    const char *p63 = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijk";
    CHECK(typeid_prefix_valid("", 0), "empty");
    CHECK(typeid_prefix_valid("a", 1), "a");
    CHECK(typeid_prefix_valid("my__type", 8), "consecutive underscores");
    CHECK(typeid_prefix_valid(p63, 63), "63 chars");
    CHECK(!typeid_prefix_valid("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl", 64),
          "64 chars");
    CHECK(!typeid_prefix_valid("_", 1), "lone underscore");
    CHECK(!typeid_prefix_valid("a1", 2), "digit");
}

static void test_errors(void) {
    typeid_t t;
    char tiny_buf[10];
    uint8_t u[16];
    CHECK(typeid_parse(&t, "prefix_8zzzzzzzzzzzzzzzzzzzzzzzzz") == TYPEID_ERR_OVERFLOW,
          "overflow error code");
    CHECK(typeid_parse(&t, "PREFIX_00000000000000000000000000") == TYPEID_ERR_PREFIX,
          "prefix error code");
    CHECK(typeid_parse(&t, "prefix_0000000000000000000000000") == TYPEID_ERR_SUFFIX,
          "suffix error code");
    CHECK(typeid_parse(NULL, "x") == TYPEID_ERR_NULL, "null");
    CHECK(typeid_parse(&t, NULL) == TYPEID_ERR_NULL, "null str");

    typeid_parse(&t, "user_01h455vb4pex5vsknk084sn02q");
    CHECK(typeid_to_string(&t, tiny_buf, sizeof tiny_buf, NULL) == TYPEID_ERR_BUFFER, "buffer");

    CHECK(typeid_uuid_parse("01890a5dac96774bbcceb302099a8057", 32, u) == TYPEID_OK,
          "uuid without hyphens");
    CHECK(typeid_uuid_parse("01890A5D-AC96-774B-BCCE-B302099A8057", 36, u) == TYPEID_OK,
          "uppercase uuid");
    CHECK(typeid_uuid_parse("01890a5d-ac96-774b-bcce-b302099a805", 35, u) == TYPEID_ERR_UUID,
          "short uuid");
    CHECK(typeid_uuid_parse("01890a5d+ac96-774b-bcce-b302099a8057", 36, u) == TYPEID_ERR_UUID,
          "bad hyphen");
    CHECK(typeid_uuid_parse("0189za5d-ac96-774b-bcce-b302099a8057", 36, u) == TYPEID_ERR_UUID,
          "bad hex");
    CHECK(strlen(typeid_strerror(TYPEID_ERR_PREFIX)) > 0, "strerror");
}

static void test_parse_n(void) {
    typeid_t t;
    const char *s = "user_01h455vb4pex5vsknk084sn02q,trailing";
    CHECK(typeid_parse_n(&t, s, 31) == TYPEID_OK, "parse_n with explicit length");
    CHECK(strcmp(t.prefix, "user") == 0, "parse_n prefix");
    CHECK(typeid_parse_n(&t, s, 32) != TYPEID_OK, "parse_n includes junk");
}

static void test_timestamp(void) {
    typeid_t t;
    typeid_parse(&t, "prefix_01h455vb4pex5vsknk084sn02q");
    /* 0x01890a5dac96 = 1688096058518 ms */
    CHECK(typeid_timestamp_ms(&t) == 0x01890a5dac96ULL, "timestamp decode");
}

static void test_sortable(void) {
    typeid_t a, b;
    char sa[TYPEID_STR_SIZE], sb[TYPEID_STR_SIZE];
    uint8_t ua[16] = {0}, ub[16] = {0};
    ua[5] = 1;
    ub[5] = 2;
    typeid_from_uuid(&a, "x", ua);
    typeid_from_uuid(&b, "x", ub);
    typeid_to_string(&a, sa, sizeof sa, NULL);
    typeid_to_string(&b, sb, sizeof sb, NULL);
    CHECK(strcmp(sa, sb) < 0, "string order should follow time order");
    CHECK(typeid_compare(&a, &b) < 0, "compare order");
}

/* Random UUIDs must round-trip, and string order must match byte order. */
static void test_roundtrip_random(void) {
    int i;
    for (i = 0; i < 20000; i++) {
        typeid_t a, b, back;
        char sa[TYPEID_STR_SIZE], sb[TYPEID_STR_SIZE];
        int bytes_cmp, str_cmp;
        typeid_new(&a, "x");
        typeid_new(&b, "x");
        a.uuid[i % 16] ^= (uint8_t)(i * 31); /* also exercise non-v7 bytes */
        typeid_to_string(&a, sa, sizeof sa, NULL);
        typeid_to_string(&b, sb, sizeof sb, NULL);
        CHECK(typeid_parse(&back, sa) == TYPEID_OK && typeid_compare(&a, &back) == 0,
              "roundtrip %s", sa);
        bytes_cmp = memcmp(a.uuid, b.uuid, 16);
        str_cmp = strcmp(sa, sb);
        CHECK((bytes_cmp < 0) == (str_cmp < 0) && (bytes_cmp == 0) == (str_cmp == 0),
              "sort order %s vs %s", sa, sb);
    }
}

int main(void) {
    test_valid_vectors();
    test_invalid_vectors();
    test_new();
    test_prefix_rules();
    test_errors();
    test_parse_n();
    test_timestamp();
    test_sortable();
    test_roundtrip_random();
    printf("%d checks, %d failures (%u valid + %u invalid spec vectors)\n", checks,
           failures, (unsigned)COUNT(VALID), (unsigned)COUNT(INVALID));
    return failures ? 1 : 0;
}
