# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-11-26

### Added

- `typeid.h`: C99 single-header implementation of TypeID specification v0.3.0.
- Generation (`typeid_new`), parsing (`typeid_parse`, `typeid_parse_n`), encoding
  (`typeid_to_string`, `typeid_from_uuid`, `typeid_from_uuid_str`) and helpers
  (`typeid_uuid_string`, `typeid_timestamp_ms`, `typeid_compare`, `typeid_prefix_valid`).
- Platform CSPRNG support for Linux, macOS, BSDs and Windows, with `TYPEID_RANDOM`
  and `TYPEID_NOW_MS` overrides.
- SQLite loadable extension with `typeid_new`, `typeid_encode`, `typeid_uuid`,
  `typeid_uuid_blob`, `typeid_prefix`, `typeid_timestamp`, `typeid_valid` and `typeid_check`.
- Example command-line tool and micro-benchmark.
- Test suite covering all official spec vectors; CI on Linux, macOS and Windows.

[Unreleased]: https://github.com/Sharkeeeeeee/typeid-c/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Sharkeeeeeee/typeid-c/releases/tag/v0.1.0
