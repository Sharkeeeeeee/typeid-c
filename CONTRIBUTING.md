# Contributing to typeid-c

Thank you for your interest in contributing. This document describes how to
report issues, propose changes and submit pull requests.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Enhancements](#suggesting-enhancements)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Commit Message Guidelines](#commit-message-guidelines)
- [Pull Request Process](#pull-request-process)
- [Updating the Spec Test Vectors](#updating-the-spec-test-vectors)

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).
By participating, you are expected to uphold it.

## Reporting Bugs

Open an issue using the **Bug report** template and include:

- The compiler, version and flags you used
- The operating system and architecture
- A minimal code sample or input string that reproduces the problem
- Expected and actual behavior

For security issues, follow [SECURITY.md](SECURITY.md) instead.

## Suggesting Enhancements

Open an issue using the **Feature request** template. Describe the use case
first, then the proposed API. Changes must stay compatible with the
[TypeID specification](https://github.com/jetify-com/typeid/tree/main/spec).

## Development Setup

Requirements: a C99 compiler, `make`, and Python 3.8+ with the standard
`sqlite3` module (for the extension tests).

```sh
git clone https://github.com/Sharkeeeeeee/typeid-c.git
cd typeid-c
make test      # library tests + SQLite extension tests
make bench     # optional micro-benchmark
```

CMake is also supported:

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

## Coding Standards

- Keep `typeid.h` a single, dependency-free header that compiles as C99 and C++11.
- Code must build cleanly with `-Wall -Wextra -Wpedantic -Werror` on GCC and Clang,
  and with MSVC.
- No dynamic memory allocation in the library.
- Public symbols use the `typeid_` prefix; internal symbols use `typeid__`.
- Every behavior change needs a test in `tests/test_typeid.c` (or
  `tests/test_sqlite.py` for the extension).
- Use 4-space indentation and keep lines under 100 characters.

## Commit Message Guidelines

This project uses [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).
Messages are written in English.

```
<type>(<optional scope>): <short summary in imperative mood>

<optional body explaining what and why>

<optional footer, e.g. BREAKING CHANGE: ... or Fixes #123>
```

| Type | Use for |
| --- | --- |
| `feat` | A new feature |
| `fix` | A bug fix |
| `docs` | Documentation only |
| `test` | Adding or fixing tests |
| `perf` | Performance improvements |
| `refactor` | Code changes that neither fix a bug nor add a feature |
| `build` | Build system (Makefile, CMake) |
| `ci` | CI configuration |
| `chore` | Maintenance tasks |

Common scopes: `core`, `sqlite`, `cli`, `windows`.

Examples:

```
feat(sqlite): add typeid_check function for CHECK constraints
fix(windows): avoid the 'small' macro from windows.h in tests
docs: document error codes in README
```

## Pull Request Process

1. Fork the repository and create a branch from `main`
   (for example `feat/monotonic-ids` or `fix/parse-overflow`).
2. Make your change, add tests, and run `make test`.
3. Update `README.md` and the `[Unreleased]` section of `CHANGELOG.md` when
   behavior or the public API changes.
4. Open a pull request using the template. Ensure CI passes on all platforms.
5. A maintainer will review your pull request. Please respond to feedback
   within a reasonable time.

## Updating the Spec Test Vectors

The files in `tests/spec/` are copied unmodified from the
[TypeID specification repository](https://github.com/jetify-com/typeid/tree/main/spec).
After updating them, regenerate the C header:

```sh
make vectors
```
