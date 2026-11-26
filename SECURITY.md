# Security Policy

## Supported Versions

| Version | Supported |
| --- | --- |
| 0.1.x | Yes |
| < 0.1 | No |

## Reporting a Vulnerability

Please **do not** report security vulnerabilities through public GitHub issues.

Instead, report them privately through
[GitHub Security Advisories](https://github.com/Sharkeeeeeee/typeid-c/security/advisories/new).

Please include:

- A description of the vulnerability and its impact
- Steps to reproduce, or a proof-of-concept input
- The affected version, compiler and platform

You can expect an initial response within 7 days. Once the issue is confirmed,
a fix will be released as soon as possible and you will be credited in the
release notes unless you prefer to remain anonymous.

## Scope

Relevant issues include memory-safety bugs in `typeid.h` or the SQLite
extension, acceptance of input the specification says must be rejected, and
weaknesses in random number generation.
