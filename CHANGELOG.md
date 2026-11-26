# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.2] - 2026-11-26

### Fixed

- Windows: the implementation no longer leaks the `min`/`max` macros from
  `<windows.h>`, which broke `std::min`, `std::max` and
  `std::numeric_limits<T>::max()` in C++ files that include it.
- CMake: C++-only projects (`project(app CXX)`) can now use the library via
  `add_subdirectory` or `FetchContent`; previously configuration failed with
  "No known features for C compiler".
- `typeid_to_string` rejects a hand-modified struct whose prefix is invalid or
  not NUL-terminated instead of producing an invalid string or reading past the
  array; `typeid_compare` no longer reads past an unterminated prefix.
- SQLite: `typeid_encode(NULL, uuid)` and `typeid_check(tid, NULL)` now return
  `NULL`, as documented, instead of treating `NULL` as an empty prefix.
- SQLite: the extension builds against SQLite headers older than 3.31.

### Changed

- Android uses `arc4random_buf`; musl-based Linux uses `getrandom` when
  `<sys/random.h>` is available, instead of reading `/dev/urandom`.
- `tid::TypedID` rejects an invalid tag prefix at compile time.
- The CMake package config is installed to `share/cmake/typeid` (it is
  architecture-independent).

## [0.1.1] - 2026-11-26

### Fixed

- Timestamps no longer drop to second precision when the implementation is
  compiled in strict ISO C mode (for example `-std=c99`) on Linux, macOS and BSD.
- `typeid new <prefix> <count>` in the example CLI now outputs IDs in strictly
  increasing order and rejects an invalid count instead of printing nothing.

### Added

- CLI test script (`make test-cli`), run in CI.

### Documentation

- README: C++ users need `typeid.hpp` as well as `typeid.h`; pin examples to
  `v0.1.0`; note `cxx_std_17` for CMake consumers; explain that the SQLite
  extension must be loaded on every writing connection, and the glibc
  requirement of the prebuilt Linux binaries.

## [0.1.0] - 2026-11-26

### Added

- `typeid.h`: C99 single-header implementation of TypeID specification v0.3.0.
- Generation (`typeid_new`), parsing (`typeid_parse`, `typeid_parse_n`), encoding
  (`typeid_to_string`, `typeid_from_uuid`, `typeid_from_uuid_str`) and helpers
  (`typeid_uuid_string`, `typeid_timestamp_ms`, `typeid_compare`, `typeid_prefix_valid`).
- Monotonic generator (`typeid_gen_t`, `typeid_gen_init`, `typeid_gen_next`) that
  guarantees strictly increasing IDs within a millisecond (RFC 9562, method 1).
- Helpers: `typeid_suffix`, `typeid_parse_expect`, `typeid_equal`, `typeid_is_nil`
  and the `TYPEID_ERR_PREFIX_MISMATCH` error code.
- `typeid.hpp`: C++17 interface with `tid::TypeID`, `tid::TypedID<Tag>` for
  compile-time prefix safety, thread-safe `tid::Generator`, `tid::Error`,
  comparison operators, `operator<<` and `std::hash`.
- Platform CSPRNG support for Linux, macOS, BSDs and Windows, with `TYPEID_RANDOM`
  and `TYPEID_NOW_MS` overrides.
- SQLite loadable extension with `typeid_new`, `typeid_encode`, `typeid_uuid`,
  `typeid_uuid_blob`, `typeid_prefix`, `typeid_suffix`, `typeid_timestamp`, `typeid_valid`
  and `typeid_check`; `typeid_new` is monotonic per connection.
- Prebuilt SQLite extension binaries for Linux, macOS and Windows on each release.
- CMake package config (`find_package(typeid)`) and pkg-config file.
- Conan 2 recipe, vcpkg overlay port and clib manifest.
- libFuzzer harness for the parser, run in CI.
- Example command-line tool and micro-benchmark.
- Test suite covering all official spec vectors; CI on Linux, macOS and Windows.

[Unreleased]: https://github.com/Sharkeeeeeee/typeid-c/compare/v0.1.2...HEAD
[0.1.2]: https://github.com/Sharkeeeeeee/typeid-c/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/Sharkeeeeeee/typeid-c/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/Sharkeeeeeee/typeid-c/releases/tag/v0.1.0
