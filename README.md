# typeid-c

**[TypeID](https://github.com/jetify-com/typeid) for C, C++ and SQLite: type-safe, K-sortable, globally unique IDs in a single header file.**

[![CI](https://github.com/Sharkeeeeeee/typeid-c/actions/workflows/ci.yml/badge.svg)](https://github.com/Sharkeeeeeee/typeid-c/actions/workflows/ci.yml)
[![TypeID spec](https://img.shields.io/badge/TypeID%20spec-v0.3.0-blue)](https://github.com/jetify-com/typeid/tree/main/spec)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-green)](LICENSE)
![C99](https://img.shields.io/badge/C-99-informational)
![C++17](https://img.shields.io/badge/C%2B%2B-17-informational)
![Header only](https://img.shields.io/badge/header--only-zero%20deps-informational)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [C++ Interface](#c-interface)
  - [Strictly Ordered IDs](#strictly-ordered-ids)
  - [Error Handling](#error-handling)
  - [Configuration](#configuration)
  - [SQLite Extension](#sqlite-extension)
  - [Command-Line Tool](#command-line-tool)
- [API Reference](#api-reference)
- [Performance](#performance)
- [Testing](#testing)
- [Design Notes](#design-notes)
- [Contributing](#contributing)
- [Security](#security)
- [Changelog](#changelog)
- [License](#license)
- [Acknowledgments](#acknowledgments)

## Overview

```
user_01h455vb4pex5vsknk084sn02q
└──┘ └────────────────────────┘
type    UUIDv7 in base32
```

TypeIDs are UUIDv7s with a type prefix, like Stripe IDs. You can't mix up a `user` ID with an `order` ID, they sort by creation time (great for database indexes), and they're 26 characters instead of 36.

## Features

- **One file.** Drop [`typeid.h`](typeid.h) into your project. No dependencies, no build system required.
- **Portable.** C99 and C++11, tested with GCC, Clang and MSVC on Linux, macOS and Windows. Uses the OS CSPRNG (`getrandom`, `arc4random_buf`, `BCryptGenRandom`).
- **Spec-compliant.** Passes every official [valid and invalid test vector](https://github.com/jetify-com/typeid/tree/main/spec) of TypeID spec v0.3.0, plus 400k+ additional checks, under ASan and UBSan, and continuously fuzzed with libFuzzer.
- **No allocations.** Everything works on caller-provided buffers. Safe to use in embedded and real-time code.
- **Idiomatic C++17 API.** [`typeid.hpp`](typeid.hpp) adds `tid::TypeID`, exceptions or `std::optional`, `std::hash`, and `tid::TypedID<Tag>` so a `UserID` can never be passed where an `OrderID` is expected, checked at compile time.
- **Strictly sortable.** An optional monotonic generator guarantees ordering even for IDs created in the same millisecond ([RFC 9562](https://www.rfc-editor.org/rfc/rfc9562#section-6.2) method 1).
- **SQLite extension included.** `SELECT typeid_new('user')` and TypeID primary keys in plain SQLite.

## Requirements

| Component | Requirement |
| --- | --- |
| C library (`typeid.h`) | Any C99 or C++11 compiler (GCC, Clang, MSVC) |
| C++ interface (`typeid.hpp`) | C++17 (GCC 8+, Clang 7+, MSVC 2019+) |
| Platforms | Linux, macOS, Windows, FreeBSD/OpenBSD/NetBSD; other targets via `TYPEID_RANDOM` and `TYPEID_NOW_MS` |
| SQLite extension | SQLite 3.31.0 or later |
| Tests | Python 3.8+ (SQLite extension tests only) |

## Installation

Pick whichever fits your project. All options provide both `typeid.h` (C) and `typeid.hpp` (C++17).

| Method | Command |
| --- | --- |
| Single file | `curl -O https://raw.githubusercontent.com/Sharkeeeeeee/typeid-c/v0.1.0/typeid.h` (C++ also needs `typeid.hpp`) |
| Conan 2 | `conan create .` from a clone, then `requires = "typeid-c/0.1.0"` |
| vcpkg | `vcpkg install typeid-c --overlay-ports=typeid-c/ports` |
| CMake FetchContent | See below |
| CMake `find_package` | `cmake --install` then `find_package(typeid)` |
| pkg-config | `cc $(pkg-config --cflags typeid) main.c` |
| clib | `clib install Sharkeeeeeee/typeid-c` |
| Git submodule | `git submodule add https://github.com/Sharkeeeeeee/typeid-c third_party/typeid-c` |
| SQLite extension | Prebuilt binaries for Linux (x86-64, ARM64), macOS (ARM64) and Windows (x86-64) on [Releases](https://github.com/Sharkeeeeeee/typeid-c/releases) |

All CMake-based methods (Conan, vcpkg, FetchContent, `find_package`) expose the same target:

```cmake
find_package(typeid CONFIG REQUIRED)   # Conan, vcpkg, or after cmake --install
target_link_libraries(your_app PRIVATE typeid::typeid)
```

**CMake FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(typeid GIT_REPOSITORY https://github.com/Sharkeeeeeee/typeid-c GIT_TAG v0.1.0)
FetchContent_MakeAvailable(typeid)
target_link_libraries(your_app PRIVATE typeid::typeid)
target_compile_features(your_app PRIVATE cxx_std_17)   # only if you use typeid.hpp
```

**Conan 2:** the recipe lives in this repository until it is published to ConanCenter.

```sh
git clone https://github.com/Sharkeeeeeee/typeid-c && conan create typeid-c
```

```ini
# conanfile.txt
[requires]
typeid-c/0.1.0

[generators]
CMakeDeps
CMakeToolchain
```

**vcpkg:** an overlay port is included until the port is accepted into the vcpkg registry.

```sh
git clone https://github.com/Sharkeeeeeee/typeid-c
vcpkg install typeid-c --overlay-ports=typeid-c/ports
```

## Usage

### Quick Start

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

### C++ Interface

[`typeid.hpp`](typeid.hpp) wraps the C library in a C++17 API. As with C, define `TYPEID_IMPLEMENTATION` in exactly one `.cpp` file.

```cpp
#define TYPEID_IMPLEMENTATION   // in exactly one .cpp file
#include "typeid.hpp"
#include <iostream>

// Compile-time type safety: each entity gets its own ID type.
struct UserTag  { static constexpr std::string_view prefix = "user"; };
struct OrderTag { static constexpr std::string_view prefix = "order"; };
using UserID  = tid::TypedID<UserTag>;
using OrderID = tid::TypedID<OrderTag>;

void ship(OrderID id);

int main() {
    UserID user = UserID::generate();
    std::cout << user << "\n";                  // user_01m8aweya5f29rna5tzgtfpbtx
    // ship(user);                               // compile error: not an OrderID

    auto id = tid::TypeID::parse("user_01h455vb4pex5vsknk084sn02q");
    std::cout << id.prefix() << " " << id.uuid() << "\n";

    if (auto maybe = OrderID::try_parse("user_01h455vb4pex5vsknk084sn02q"); !maybe) {
        std::cout << "not an order ID\n";       // wrong prefix -> std::nullopt
    }

    tid::Generator gen;                          // thread-safe, monotonic
    OrderID a = OrderID::generate(gen), b = OrderID::generate(gen);  // a < b
}
```

| C++ API | Description |
| --- | --- |
| `tid::TypeID::generate(prefix)` | New TypeID with a random UUIDv7 |
| `tid::TypeID::parse(str[, prefix])` | Parse; throws `tid::Error` (with `code()`) on invalid input |
| `tid::TypeID::try_parse(str)` | Parse; returns `std::optional<TypeID>`, never throws |
| `tid::TypeID::from_uuid(prefix, uuid)` / `from_bytes(prefix, bytes)` | Wrap an existing UUID |
| `.prefix()` `.suffix()` `.to_string()` `.uuid()` `.uuid_bytes()` | Accessors |
| `.timestamp()` / `.timestamp_ms()` | `std::chrono::system_clock::time_point` / Unix ms |
| `== != < <= > >=`, `operator<<`, `std::hash` | Comparison, streaming, use as map key |
| `tid::Generator::next(prefix)` | Thread-safe monotonic generation |
| `tid::TypedID<Tag>` | Same API with the prefix fixed at compile time |

### Strictly Ordered IDs

```c
typeid_gen_t gen;
typeid_gen_init(&gen);

typeid_t a, b;
typeid_gen_next(&gen, &a, "event");
typeid_gen_next(&gen, &b, "event");
/* a < b, even within the same millisecond */
```

### Error Handling

Every function that can fail returns a `typeid_err`. Use `typeid_strerror()` for a readable message:

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

### SQLite Extension

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
| `typeid_new([prefix])` | A new TypeID (strictly increasing within a connection) |
| `typeid_encode(prefix, uuid)` | TypeID from a UUID given as text or a 16-byte blob |
| `typeid_uuid(tid)` / `typeid_uuid_blob(tid)` | The UUID as text / as a 16-byte blob |
| `typeid_prefix(tid)` / `typeid_suffix(tid)` | The type prefix / the 26-char suffix |
| `typeid_timestamp(tid)` | Creation time in Unix ms |
| `typeid_valid(tid)` | 1 if valid, else 0 |
| `typeid_check(tid, prefix)` | 1 if valid and has that prefix, else 0 (great in `CHECK` constraints) |

Invalid input raises an SQL error. `NULL` in gives `NULL` out.

> [!NOTE]
> The functions exist only on connections that have loaded the extension. If a table uses `typeid_new()` in a `DEFAULT` or `typeid_check()` in a `CHECK` constraint, load the extension on every connection that writes to it, or inserts fail with `no such function`.
>
> The prebuilt Linux binaries require glibc 2.25 or later (Ubuntu 18.04+, Debian 10+, RHEL 8+). On Alpine or older systems, build from source with `make typeid.so`.

### Command-Line Tool

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

## API Reference

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
| `typeid_parse_expect(&id, str, "user")` | Parse and require a specific prefix (`TYPEID_ERR_PREFIX_MISMATCH` otherwise). |
| `typeid_suffix(&id, buf)` | The 26-char base32 suffix. |
| `typeid_equal(&a, &b)` / `typeid_is_nil(&id)` | Equality / nil-UUID check. |
| `typeid_gen_init(&gen)` / `typeid_gen_next(&gen, &id, "user")` | Monotonic generator: every ID sorts after the previous one. |
| `typeid_encode_suffix` / `typeid_decode_suffix` | Low-level base32 encode/decode of the 26-char suffix. |

The struct is plain data, so you can copy it, store it, and compare it with `typeid_compare`:

```c
typedef struct typeid_t {
    char prefix[64];   /* NUL-terminated, may be empty */
    uint8_t uuid[16];  /* big-endian UUID bytes */
} typeid_t;
```

### Error Codes

| Code | Value | Meaning |
| --- | --- | --- |
| `TYPEID_OK` | 0 | Success |
| `TYPEID_ERR_NULL` | 1 | A required pointer argument was `NULL` |
| `TYPEID_ERR_PREFIX` | 2 | Prefix is not `[a-z_]`, exceeds 63 chars, or starts/ends with `_` |
| `TYPEID_ERR_SUFFIX` | 3 | Suffix is not 26 chars of the base32 alphabet |
| `TYPEID_ERR_OVERFLOW` | 4 | Suffix encodes more than 128 bits (first char > `7`) |
| `TYPEID_ERR_UUID` | 5 | Malformed UUID string |
| `TYPEID_ERR_BUFFER` | 6 | Output buffer is too small |
| `TYPEID_ERR_RANDOM` | 7 | The system random number generator failed |
| `TYPEID_ERR_PREFIX_MISMATCH` | 8 | Valid TypeID, but not the prefix passed to `typeid_parse_expect` |

## Performance

`make bench` on an x86-64 Linux VM, GCC 13, `-O2`:

| Operation | Time |
| --- | --- |
| `typeid_to_string` | ~30 ns |
| `typeid_parse` (validate + decode) | ~50 ns |
| `typeid_new` (one `getrandom` syscall) | ~330 ns |
| `typeid_gen_next` (monotonic) | ~350 ns |

## Testing

```sh
make test     # C tests + C++ tests + SQLite extension tests
```

Run `make fuzz` (requires clang) to fuzz the parser locally.

The tests run every case from the official [`valid.json`](tests/spec/valid.json) and [`invalid.json`](tests/spec/invalid.json), round-trip 20,000 random UUIDs, and check that string order always matches byte (time) order. CI runs them on Linux, macOS and Windows with GCC, Clang, MSVC, C++ mode and sanitizers.

## Design Notes

- `typeid_new` is thread-safe and stateless. IDs it generates in the same millisecond are ordered randomly relative to each other. When you need strict ordering, use a `typeid_gen_t` generator (one per thread, or guard a shared one with a mutex). The SQLite extension keeps one generator per connection.
- Decoding accepts any UUID version, as the spec recommends, so you can wrap existing UUIDv4 keys. Only `typeid_new` guarantees UUIDv7.
- Timestamps have millisecond precision on Linux, macOS, the BSDs and Windows in every language mode, including strict `-std=c99`. On other platforms without POSIX or C11 clocks, define `TYPEID_NOW_MS()`.

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) for the development workflow, coding style and commit message conventions ([Conventional Commits](https://www.conventionalcommits.org/)). This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).

## Security

Please do not report security vulnerabilities through public issues. See [SECURITY.md](SECURITY.md) for how to report them privately.

## Changelog

All notable changes are documented in [CHANGELOG.md](CHANGELOG.md). This project follows [Semantic Versioning](https://semver.org/).

## License

Distributed under the [Apache License 2.0](LICENSE). See [NOTICE](NOTICE) for attribution.

## Acknowledgments

- TypeID was designed by [Jetify](https://github.com/jetify-com/typeid). This is an independent implementation of their [specification](https://github.com/jetify-com/typeid/tree/main/spec).
- The test vectors in [`tests/spec/`](tests/spec) are copied unmodified from the specification repository.
