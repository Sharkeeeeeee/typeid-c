# typeid-c

**[TypeID](https://github.com/jetify-com/typeid) for C and SQLite: type-safe, K-sortable, globally unique IDs in a single header file.**

[![CI](https://github.com/Sharkeeeeeee/typeid-c/actions/workflows/ci.yml/badge.svg)](https://github.com/Sharkeeeeeee/typeid-c/actions/workflows/ci.yml)
[![TypeID spec](https://img.shields.io/badge/TypeID%20spec-v0.3.0-blue)](https://github.com/jetify-com/typeid/tree/main/spec)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-green)](LICENSE)
![C99](https://img.shields.io/badge/C-99-informational)
![Header only](https://img.shields.io/badge/header--only-zero%20deps-informational)

```
user_01h455vb4pex5vsknk084sn02q
└──┘ └────────────────────────┘
type    UUIDv7 in base32
```

TypeIDs are UUIDv7s with a type prefix, like Stripe IDs. You can't mix up a `user` ID with an `order` ID, they sort by creation time (great for database indexes), and they're 26 characters instead of 36.

- **One file.** Drop [`typeid.h`](typeid.h) into your project. No dependencies, no build system required.
- **Portable.** C99 and C++11, tested with GCC, Clang and MSVC on Linux, macOS and Windows. Uses the OS CSPRNG (`getrandom`, `arc4random_buf`, `BCryptGenRandom`).
- **Spec-compliant.** Passes every official [valid and invalid test vector](https://github.com/jetify-com/typeid/tree/main/spec) of TypeID spec v0.3.0, plus 40k+ additional checks, under ASan and UBSan.
- **No allocations.** Everything works on caller-provided buffers. Safe to use in embedded and real-time code.
- **SQLite extension included.** `SELECT typeid_new('user')` and TypeID primary keys in plain SQLite.

## Quick start

```c
#define TYPEID_IMPLEMENTATION   /* in exactly one .c file */
#include "typeid.h"
#include <stdio.h>

int main(void) {
    typeid_t id;
    char str[TYPEID_STR_SIZE];

    typeid_new(&id, "user");
    typeid_to_string(&id, str, sizeof str, NULL);
    printf("%s\n", str);                  /* user_01m89p3fzde1ftq481qavnfwaz */

    if (typeid_parse(&id, "user_01h455vb4pex5vsknk084sn02q") == TYPEID_OK) {
        char uuid[TYPEID_UUID_STR_SIZE];
        typeid_uuid_string(&id, uuid);
        printf("%s %s\n", id.prefix, uuid); /* user 01890a5d-ac96-774b-bcce-b302099a8057 */
    }
    return 0;
}
```

```sh
cc -O2 main.c -o main && ./main
```

### Install

Pick whichever fits your project:

- **Copy the file:** `curl -O https://raw.githubusercontent.com/Sharkeeeeeee/typeid-c/main/typeid.h`
- **CMake FetchContent:**

  ```cmake
  include(FetchContent)
  FetchContent_Declare(typeid GIT_REPOSITORY https://github.com/Sharkeeeeeee/typeid-c GIT_TAG main)
  FetchContent_MakeAvailable(typeid)
  target_link_libraries(your_app PRIVATE typeid::typeid)
  ```

- **Git submodule:** `git submodule add https://github.com/Sharkeeeeeee/typeid-c third_party/typeid-c`

## API

All functions return `TYPEID_OK` (0) on success or a `typeid_err` code. `typeid_strerror()` turns a code into a message.

| Function | What it does |
| --- | --- |
| `typeid_new(&id, "user")` | New TypeID with a fresh UUIDv7. Prefix may be `""` or `NULL`. |
| `typeid_parse(&id, str)` | Parse and validate a string. `typeid_parse_n` takes an explicit length. |
| `typeid_to_string(&id, buf, size, &len)` | Write the canonical string. `TYPEID_STR_SIZE` (91) always fits. |
| `typeid_from_uuid(&id, "user", bytes)` | Wrap an existing 16-byte UUID (any version). |
| `typeid_from_uuid_str(&id, "user", "0189...")` | Same, from a UUID string (hyphens optional). |
| `typeid_uuid_string(&id, buf)` | UUID as `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`. |
| `typeid_timestamp_ms(&id)` | Creation time (Unix ms) stored in the UUIDv7. |
| `typeid_compare(&a, &b)` | Order by prefix, then by UUID (time). |
| `typeid_prefix_valid(p, len)` | Check a prefix against `^([a-z]([a-z_]{0,61}[a-z])?)?$`. |
| `typeid_encode_suffix` / `typeid_decode_suffix` | Low-level base32 encode/decode of the 26-char suffix. |

The struct is plain data, so you can copy it, store it, and compare it with `typeid_compare`:

```c
typedef struct typeid_t {
    char prefix[64];   /* NUL-terminated, may be empty */
    uint8_t uuid[16];  /* big-endian UUID bytes */
} typeid_t;
```

Error codes tell you exactly what's wrong:

```c
typeid_parse(&id, "User_01h455vb4pex5vsknk084sn02q");  /* TYPEID_ERR_PREFIX   */
typeid_parse(&id, "user_01h455vb4pex5vsknk084sn0");    /* TYPEID_ERR_SUFFIX   */
typeid_parse(&id, "user_8zzzzzzzzzzzzzzzzzzzzzzzzz");  /* TYPEID_ERR_OVERFLOW */
```

### Configuration

Define these before including the header in your implementation file:

| Macro | Effect |
| --- | --- |
| `TYPEID_STATIC` | Make all functions `static` (private to one translation unit). |
| `TYPEID_RANDOM(buf, n)` | Use your own CSPRNG. Must return 0 on success. |
| `TYPEID_NOW_MS()` | Use your own clock (Unix ms, `uint64_t`). Handy for tests and embedded targets. |

## SQLite extension

[`sqlite/typeid_sqlite.c`](sqlite/typeid_sqlite.c) is a loadable extension built on the same header.

```sh
make typeid.so        # or typeid.dylib on macOS
sqlite3
```

```sql
.load ./typeid

CREATE TABLE users (
  id   TEXT PRIMARY KEY DEFAULT (typeid_new('user')) CHECK (typeid_check(id, 'user')),
  name TEXT NOT NULL
);

INSERT INTO users(name) VALUES ('Ada');
SELECT id, typeid_uuid(id), datetime(typeid_timestamp(id) / 1000, 'unixepoch') FROM users;
-- user_01m89p3fzde1ftq481qavnfwaz | 01a21361-bfed-705f-ab91-01bab757f15f | 2026-11-25 15:45:02
```

| Function | Returns |
| --- | --- |
| `typeid_new([prefix])` | A new TypeID |
| `typeid_encode(prefix, uuid)` | TypeID from a UUID given as text or a 16-byte blob |
| `typeid_uuid(tid)` / `typeid_uuid_blob(tid)` | The UUID as text / as a 16-byte blob |
| `typeid_prefix(tid)` | The type prefix |
| `typeid_timestamp(tid)` | Creation time in Unix ms |
| `typeid_valid(tid)` | 1 if valid, else 0 |
| `typeid_check(tid, prefix)` | 1 if valid and has that prefix, else 0 (great in `CHECK` constraints) |

Invalid input raises an SQL error. `NULL` in gives `NULL` out.

## Command-line tool

```console
$ make typeid
$ ./typeid new user 3
user_01m89p3fzde1ftq481qavnfwaz
...
$ ./typeid decode prefix_01h455vb4pex5vsknk084sn02q
type: prefix
uuid: 01890a5d-ac96-774b-bcce-b302099a8057
time: 1688096058518
$ ./typeid encode prefix 01890a5d-ac96-774b-bcce-b302099a8057
prefix_01h455vb4pex5vsknk084sn02q
```

## Performance

`make bench` on an x86-64 Linux VM, GCC 13, `-O2`:

| Operation | Time |
| --- | --- |
| `typeid_to_string` | ~45 ns |
| `typeid_parse` (validate + decode) | ~80 ns |
| `typeid_new` (one `getrandom` syscall) | ~400 ns |

## Testing

```sh
make test     # C tests + SQLite extension tests
```

The tests run every case from the official [`valid.json`](tests/spec/valid.json) and [`invalid.json`](tests/spec/invalid.json), round-trip 20,000 random UUIDs, and check that string order always matches byte (time) order. CI runs them on Linux, macOS and Windows with GCC, Clang, MSVC, C++ mode and sanitizers.

## Notes

- `typeid_new` is thread-safe. IDs generated in the same millisecond are ordered randomly relative to each other (the spec does not require monotonic ordering within a millisecond).
- Decoding accepts any UUID version, as the spec recommends, so you can wrap existing UUIDv4 keys. Only `typeid_new` guarantees UUIDv7.
- If you compile the implementation with strict `-std=c99` and no POSIX feature macros, timestamps fall back to second precision. Add `-D_POSIX_C_SOURCE=200809L` (or use the default `gnu` modes) to get milliseconds.

## Credits and license

TypeID was designed by [Jetify](https://github.com/jetify-com/typeid). This is an independent implementation of their [specification](https://github.com/jetify-com/typeid/tree/main/spec). The test vectors in `tests/spec/` are copied from the spec repository.

Licensed under the [Apache License 2.0](LICENSE). See [NOTICE](NOTICE).
