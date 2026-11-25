/*
 * typeid_sqlite.c - TypeID functions for SQLite, as a loadable extension.
 *
 * Build:
 *   Linux:   gcc -O2 -fPIC -shared sqlite/typeid_sqlite.c -o typeid.so
 *   macOS:   gcc -O2 -fPIC -dynamiclib sqlite/typeid_sqlite.c -o typeid.dylib
 *   Windows: cl /O2 sqlite\typeid_sqlite.c /LD /Fe:typeid.dll
 *
 * Load:
 *   sqlite> .load ./typeid
 *
 * Functions (NULL in, NULL out):
 *   typeid_new([prefix])         new TypeID with a fresh UUIDv7
 *   typeid_encode(prefix, uuid)  TypeID from a UUID (text or 16-byte blob)
 *   typeid_uuid(tid)             UUID as canonical text
 *   typeid_uuid_blob(tid)        UUID as a 16-byte blob
 *   typeid_prefix(tid)           type prefix ('' when there is none)
 *   typeid_timestamp(tid)        UUIDv7 Unix timestamp in milliseconds
 *   typeid_valid(tid)            1 if tid is a valid TypeID, else 0
 *   typeid_check(tid, prefix)    1 if tid is valid and has this prefix, else 0
 *
 * Invalid input raises an SQL error, except in typeid_valid/typeid_check.
 */
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

#define TYPEID_STATIC
#define TYPEID_IMPLEMENTATION
#include "../typeid.h"

#include <string.h>

static void tid_error(sqlite3_context *ctx, const char *fn, typeid_err e) {
    char *msg = sqlite3_mprintf("%s: %s", fn, typeid_strerror(e));
    if (msg) {
        sqlite3_result_error(ctx, msg, -1);
        sqlite3_free(msg);
    } else {
        sqlite3_result_error_nomem(ctx);
    }
}

static void tid_result_text(sqlite3_context *ctx, const typeid_t *tid) {
    char buf[TYPEID_STR_SIZE];
    size_t len;
    typeid_to_string(tid, buf, sizeof buf, &len);
    sqlite3_result_text(ctx, buf, (int)len, SQLITE_TRANSIENT);
}

/* Reads a text argument into a NUL-terminated prefix buffer. Returns 0 if the
 * argument is too long to be a valid prefix. */
static int tid_arg_prefix(sqlite3_value *v, char out[TYPEID_PREFIX_MAX + 1]) {
    const unsigned char *p = sqlite3_value_text(v);
    int n = sqlite3_value_bytes(v);
    if (!p || n <= 0) {
        out[0] = '\0';
        return 1;
    }
    if (n > TYPEID_PREFIX_MAX) return 0;
    memcpy(out, p, (size_t)n);
    out[n] = '\0';
    return strlen(out) == (size_t)n; /* reject embedded NULs */
}

/* Parses the TypeID argument. Returns TYPEID_OK, or an error code. */
static typeid_err tid_arg_parse(sqlite3_value *v, typeid_t *out) {
    const unsigned char *p = sqlite3_value_text(v);
    int n = sqlite3_value_bytes(v);
    if (!p) return TYPEID_ERR_NULL;
    return typeid_parse_n(out, (const char *)p, (size_t)n);
}

static void fn_new(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    char prefix[TYPEID_PREFIX_MAX + 1];
    typeid_t tid;
    typeid_err e;
    if (argc == 1 && sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        prefix[0] = '\0';
    } else if (argc == 1 && !tid_arg_prefix(argv[0], prefix)) {
        tid_error(ctx, "typeid_new", TYPEID_ERR_PREFIX);
        return;
    } else if (argc == 0) {
        prefix[0] = '\0';
    }
    e = typeid_new(&tid, prefix);
    if (e != TYPEID_OK) {
        tid_error(ctx, "typeid_new", e);
        return;
    }
    tid_result_text(ctx, &tid);
}

static void fn_encode(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    char prefix[TYPEID_PREFIX_MAX + 1];
    uint8_t uuid[16];
    typeid_t tid;
    typeid_err e;
    (void)argc;
    if (sqlite3_value_type(argv[1]) == SQLITE_NULL) return;
    if (!tid_arg_prefix(argv[0], prefix)) {
        tid_error(ctx, "typeid_encode", TYPEID_ERR_PREFIX);
        return;
    }
    if (sqlite3_value_type(argv[1]) == SQLITE_BLOB) {
        if (sqlite3_value_bytes(argv[1]) != 16) {
            tid_error(ctx, "typeid_encode", TYPEID_ERR_UUID);
            return;
        }
        memcpy(uuid, sqlite3_value_blob(argv[1]), 16);
    } else {
        const char *s = (const char *)sqlite3_value_text(argv[1]);
        e = typeid_uuid_parse(s, (size_t)sqlite3_value_bytes(argv[1]), uuid);
        if (e != TYPEID_OK) {
            tid_error(ctx, "typeid_encode", e);
            return;
        }
    }
    e = typeid_from_uuid(&tid, prefix, uuid);
    if (e != TYPEID_OK) {
        tid_error(ctx, "typeid_encode", e);
        return;
    }
    tid_result_text(ctx, &tid);
}

#define TID_ARG_OR_RETURN(fn)                                   \
    typeid_t tid;                                               \
    typeid_err e;                                               \
    (void)argc;                                                 \
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) return;     \
    e = tid_arg_parse(argv[0], &tid);                           \
    if (e != TYPEID_OK) {                                       \
        tid_error(ctx, fn, e);                                  \
        return;                                                 \
    }

static void fn_uuid(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    char buf[TYPEID_UUID_STR_SIZE];
    TID_ARG_OR_RETURN("typeid_uuid")
    typeid_uuid_string(&tid, buf);
    sqlite3_result_text(ctx, buf, 36, SQLITE_TRANSIENT);
}

static void fn_uuid_blob(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    TID_ARG_OR_RETURN("typeid_uuid_blob")
    sqlite3_result_blob(ctx, tid.uuid, 16, SQLITE_TRANSIENT);
}

static void fn_prefix(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    TID_ARG_OR_RETURN("typeid_prefix")
    sqlite3_result_text(ctx, tid.prefix, -1, SQLITE_TRANSIENT);
}

static void fn_timestamp(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    TID_ARG_OR_RETURN("typeid_timestamp")
    sqlite3_result_int64(ctx, (sqlite3_int64)typeid_timestamp_ms(&tid));
}

static void fn_valid(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    typeid_t tid;
    (void)argc;
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) return;
    sqlite3_result_int(ctx, tid_arg_parse(argv[0], &tid) == TYPEID_OK);
}

static void fn_check(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    typeid_t tid;
    const unsigned char *want;
    (void)argc;
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) return;
    want = sqlite3_value_text(argv[1]);
    sqlite3_result_int(ctx, tid_arg_parse(argv[0], &tid) == TYPEID_OK && want &&
                                strcmp(tid.prefix, (const char *)want) == 0);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_typeid_init(sqlite3 *db, char **pzErrMsg,
                        const sqlite3_api_routines *pApi) {
    static const struct {
        const char *name;
        int nargs;
        int deterministic;
        void (*fn)(sqlite3_context *, int, sqlite3_value **);
    } fns[] = {
        {"typeid_new", 0, 0, fn_new},
        {"typeid_new", 1, 0, fn_new},
        {"typeid_encode", 2, 1, fn_encode},
        {"typeid_uuid", 1, 1, fn_uuid},
        {"typeid_uuid_blob", 1, 1, fn_uuid_blob},
        {"typeid_prefix", 1, 1, fn_prefix},
        {"typeid_timestamp", 1, 1, fn_timestamp},
        {"typeid_valid", 1, 1, fn_valid},
        {"typeid_check", 2, 1, fn_check},
    };
    size_t i;
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    (void)pzErrMsg;
    for (i = 0; i < sizeof fns / sizeof fns[0] && rc == SQLITE_OK; i++) {
        int flags = SQLITE_UTF8 | SQLITE_INNOCUOUS;
        if (fns[i].deterministic) flags |= SQLITE_DETERMINISTIC;
        rc = sqlite3_create_function(db, fns[i].name, fns[i].nargs, flags, NULL,
                                     fns[i].fn, NULL, NULL);
    }
    return rc;
}
