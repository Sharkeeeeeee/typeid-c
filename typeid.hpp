/*
 * typeid.hpp - Modern C++17 interface for typeid.h.
 * https://github.com/Sharkeeeeeee/typeid-c
 *
 * USAGE
 *   In exactly ONE .cpp file, define TYPEID_IMPLEMENTATION before including:
 *
 *       #define TYPEID_IMPLEMENTATION
 *       #include "typeid.hpp"
 *
 *   Every other file just includes "typeid.hpp". typeid.h must be on the
 *   include path next to this header.
 *
 * EXAMPLE
 *   tid::TypeID id = tid::TypeID::generate("user");
 *   std::cout << id << "\n";                     // user_01h455vb4pex5vsknk084sn02q
 *
 *   struct UserTag { static constexpr std::string_view prefix = "user"; };
 *   using UserID = tid::TypedID<UserTag>;
 *   UserID u = UserID::parse("user_01h455vb4pex5vsknk084sn02q");
 *   // UserID::parse("post_...") throws tid::Error (prefix mismatch)
 *
 * LICENSE
 *   Apache License 2.0. See LICENSE.
 */
#ifndef TYPEID_HPP
#define TYPEID_HPP

#if !defined(__cplusplus) || (__cplusplus < 201703L && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L))
#error "typeid.hpp requires C++17 or later"
#endif

/* typeid.h is C: keep C-style casts from tripping strict C++ warnings. */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#endif
#include "typeid.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace tid {

/* Error codes, mirroring typeid_err. */
enum class Errc {
    ok = TYPEID_OK,
    null_argument = TYPEID_ERR_NULL,
    invalid_prefix = TYPEID_ERR_PREFIX,
    invalid_suffix = TYPEID_ERR_SUFFIX,
    overflow = TYPEID_ERR_OVERFLOW,
    invalid_uuid = TYPEID_ERR_UUID,
    buffer_too_small = TYPEID_ERR_BUFFER,
    random_failure = TYPEID_ERR_RANDOM,
    prefix_mismatch = TYPEID_ERR_PREFIX_MISMATCH,
};

/* Thrown by the throwing APIs. code() tells you what went wrong. */
class Error : public std::invalid_argument {
public:
    explicit Error(typeid_err err)
        : std::invalid_argument(typeid_strerror(err)), code_(static_cast<Errc>(err)) {}
    Errc code() const noexcept { return code_; }

private:
    Errc code_;
};

using UUIDBytes = std::array<std::uint8_t, 16>;

namespace detail {
/* constexpr version of typeid_prefix_valid, for compile-time checks. */
constexpr bool prefix_valid(std::string_view p) noexcept {
    if (p.empty()) return true;
    if (p.size() > TYPEID_PREFIX_MAX || p.front() == '_' || p.back() == '_') return false;
    for (char c : p)
        if (c != '_' && (c < 'a' || c > 'z')) return false;
    return true;
}

inline void check(typeid_err err) {
    if (err != TYPEID_OK) throw Error(err);
}

/* Prefixes are at most 63 chars; copy into a NUL-terminated buffer. */
struct PrefixBuf {
    char data[TYPEID_PREFIX_MAX + 1];
    typeid_err err = TYPEID_OK;
    explicit PrefixBuf(std::string_view p) noexcept {
        if (p.size() > TYPEID_PREFIX_MAX || !typeid_prefix_valid(p.data(), p.size())) {
            err = TYPEID_ERR_PREFIX;
            data[0] = '\0';
            return;
        }
        if (!p.empty()) std::memcpy(data, p.data(), p.size());
        data[p.size()] = '\0';
    }
};
} // namespace detail

/* A TypeID: a type prefix plus a UUID. Cheap to copy (80 bytes). */
class TypeID {
public:
    /* The nil TypeID: empty prefix, all-zero UUID. */
    TypeID() noexcept : raw_{} {}

    explicit TypeID(const typeid_t &raw) noexcept : raw_(raw) {}

    /* Generate a new TypeID with a fresh random UUIDv7. */
    static TypeID generate(std::string_view prefix = {}) {
        detail::PrefixBuf p(prefix);
        detail::check(p.err);
        TypeID id;
        detail::check(typeid_new(&id.raw_, p.data));
        return id;
    }

    /* Parse a TypeID string. Throws tid::Error on invalid input. */
    static TypeID parse(std::string_view str) {
        TypeID id;
        detail::check(typeid_parse_n(&id.raw_, str.data(), str.size()));
        return id;
    }

    /* Parse a TypeID string. Returns std::nullopt on invalid input. */
    static std::optional<TypeID> try_parse(std::string_view str) noexcept {
        TypeID id;
        if (typeid_parse_n(&id.raw_, str.data(), str.size()) != TYPEID_OK) return std::nullopt;
        return id;
    }

    /* Parse and require a specific prefix. Throws Errc::prefix_mismatch. */
    static TypeID parse(std::string_view str, std::string_view expected_prefix) {
        TypeID id = parse(str);
        if (id.prefix() != expected_prefix) throw Error(TYPEID_ERR_PREFIX_MISMATCH);
        return id;
    }

    /* Build from a UUID string ("0189...-...", hyphens optional). */
    static TypeID from_uuid(std::string_view prefix, std::string_view uuid) {
        detail::PrefixBuf p(prefix);
        detail::check(p.err);
        UUIDBytes bytes;
        detail::check(typeid_uuid_parse(uuid.data(), uuid.size(), bytes.data()));
        return from_bytes(prefix, bytes);
    }

    /* Build from 16 raw UUID bytes (big-endian). */
    static TypeID from_bytes(std::string_view prefix, const UUIDBytes &uuid) {
        detail::PrefixBuf p(prefix);
        detail::check(p.err);
        TypeID id;
        detail::check(typeid_from_uuid(&id.raw_, p.data, uuid.data()));
        return id;
    }

    std::string_view prefix() const noexcept { return raw_.prefix; }

    std::string suffix() const {
        char buf[TYPEID_SUFFIX_LEN + 1];
        typeid_suffix(&raw_, buf);
        return std::string(buf, TYPEID_SUFFIX_LEN);
    }

    std::string to_string() const {
        char buf[TYPEID_STR_SIZE];
        std::size_t len = 0;
        typeid_to_string(&raw_, buf, sizeof buf, &len);
        return std::string(buf, len);
    }

    std::string uuid() const {
        char buf[TYPEID_UUID_STR_SIZE];
        typeid_uuid_string(&raw_, buf);
        return std::string(buf, 36);
    }

    UUIDBytes uuid_bytes() const noexcept {
        UUIDBytes out;
        std::memcpy(out.data(), raw_.uuid, 16);
        return out;
    }

    std::uint64_t timestamp_ms() const noexcept { return typeid_timestamp_ms(&raw_); }

    std::chrono::system_clock::time_point timestamp() const {
        return std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::milliseconds(timestamp_ms())));
    }

    bool is_nil() const noexcept { return typeid_is_nil(&raw_) != 0; }

    const typeid_t &raw() const noexcept { return raw_; }

    friend bool operator==(const TypeID &a, const TypeID &b) noexcept {
        return typeid_compare(&a.raw_, &b.raw_) == 0;
    }
    friend bool operator!=(const TypeID &a, const TypeID &b) noexcept { return !(a == b); }
    friend bool operator<(const TypeID &a, const TypeID &b) noexcept {
        return typeid_compare(&a.raw_, &b.raw_) < 0;
    }
    friend bool operator>(const TypeID &a, const TypeID &b) noexcept { return b < a; }
    friend bool operator<=(const TypeID &a, const TypeID &b) noexcept { return !(b < a); }
    friend bool operator>=(const TypeID &a, const TypeID &b) noexcept { return !(a < b); }

    friend std::ostream &operator<<(std::ostream &os, const TypeID &id) {
        return os << id.to_string();
    }

private:
    friend class Generator;
    typeid_t raw_;
};

/* Monotonic generator: every ID it returns sorts after the previous one,
 * even within the same millisecond. Thread-safe. */
class Generator {
public:
    Generator() noexcept { typeid_gen_init(&gen_); }
    Generator(const Generator &) = delete;
    Generator &operator=(const Generator &) = delete;

    TypeID next(std::string_view prefix = {}) {
        detail::PrefixBuf p(prefix);
        detail::check(p.err);
        TypeID id;
        std::lock_guard<std::mutex> lock(mutex_);
        detail::check(typeid_gen_next(&gen_, &id.raw_, p.data));
        return id;
    }

private:
    std::mutex mutex_;
    typeid_gen_t gen_;
};

/* A TypeID whose prefix is fixed at compile time, so a UserID can never be
 * passed where an OrderID is expected.
 *
 *   struct UserTag { static constexpr std::string_view prefix = "user"; };
 *   using UserID = tid::TypedID<UserTag>;
 */
template <typename Tag>
class TypedID {
public:
    static constexpr std::string_view prefix_v = Tag::prefix;
    static_assert(detail::prefix_valid(prefix_v),
                  "TypedID prefix must be at most 63 chars of [a-z_], starting and "
                  "ending with a letter");

    /* The nil ID with this type's prefix. */
    TypedID() : id_(TypeID::from_bytes(prefix_v, UUIDBytes{})) {}

    static TypedID generate() { return TypedID(TypeID::generate(prefix_v)); }

    static TypedID generate(Generator &gen) { return TypedID(gen.next(prefix_v)); }

    /* Throws tid::Error, including Errc::prefix_mismatch for another type. */
    static TypedID parse(std::string_view str) {
        return TypedID(TypeID::parse(str, prefix_v));
    }

    static std::optional<TypedID> try_parse(std::string_view str) noexcept {
        auto id = TypeID::try_parse(str);
        if (!id || id->prefix() != prefix_v) return std::nullopt;
        return TypedID(*id);
    }

    static TypedID from_uuid(std::string_view uuid) {
        return TypedID(TypeID::from_uuid(prefix_v, uuid));
    }

    static TypedID from_bytes(const UUIDBytes &uuid) {
        return TypedID(TypeID::from_bytes(prefix_v, uuid));
    }

    static constexpr std::string_view prefix() noexcept { return prefix_v; }
    std::string suffix() const { return id_.suffix(); }
    std::string to_string() const { return id_.to_string(); }
    std::string uuid() const { return id_.uuid(); }
    UUIDBytes uuid_bytes() const noexcept { return id_.uuid_bytes(); }
    std::uint64_t timestamp_ms() const noexcept { return id_.timestamp_ms(); }
    std::chrono::system_clock::time_point timestamp() const { return id_.timestamp(); }
    bool is_nil() const noexcept { return id_.is_nil(); }

    /* Untyped view, e.g. for logging or storage layers. */
    const TypeID &untyped() const noexcept { return id_; }

    friend bool operator==(const TypedID &a, const TypedID &b) noexcept { return a.id_ == b.id_; }
    friend bool operator!=(const TypedID &a, const TypedID &b) noexcept { return a.id_ != b.id_; }
    friend bool operator<(const TypedID &a, const TypedID &b) noexcept { return a.id_ < b.id_; }
    friend bool operator>(const TypedID &a, const TypedID &b) noexcept { return a.id_ > b.id_; }
    friend bool operator<=(const TypedID &a, const TypedID &b) noexcept { return a.id_ <= b.id_; }
    friend bool operator>=(const TypedID &a, const TypedID &b) noexcept { return a.id_ >= b.id_; }

    friend std::ostream &operator<<(std::ostream &os, const TypedID &id) { return os << id.id_; }

private:
    explicit TypedID(const TypeID &id) : id_(id) {}
    TypeID id_;
};

} // namespace tid

namespace std {
template <>
struct hash<tid::TypeID> {
    std::size_t operator()(const tid::TypeID &id) const noexcept {
        /* FNV-1a over prefix and UUID bytes. */
        std::uint64_t h = 1469598103934665603ULL;
        for (char c : id.prefix()) h = (h ^ static_cast<unsigned char>(c)) * 1099511628211ULL;
        h = (h ^ 0xFFu) * 1099511628211ULL; /* separator */
        for (std::uint8_t b : id.raw().uuid) h = (h ^ b) * 1099511628211ULL;
        return static_cast<std::size_t>(h);
    }
};

template <typename Tag>
struct hash<tid::TypedID<Tag>> {
    std::size_t operator()(const tid::TypedID<Tag> &id) const noexcept {
        return hash<tid::TypeID>()(id.untyped());
    }
};
} // namespace std

#endif /* TYPEID_HPP */
