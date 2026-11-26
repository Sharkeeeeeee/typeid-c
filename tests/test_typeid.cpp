// Tests for the C++17 interface (typeid.hpp).
#define TYPEID_IMPLEMENTATION
#include "../typeid.hpp"
#include "vectors.h"

#include <algorithm>
#include <limits>
#include <iostream>
#include <set>
#include <sstream>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++checks;                                                            \
        if (!(cond)) {                                                       \
            ++failures;                                                      \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << ": " #cond \
                      << "\n";                                               \
        }                                                                    \
    } while (0)

template <typename F>
static bool throws_code(F &&f, tid::Errc code) {
    try {
        f();
    } catch (const tid::Error &e) {
        return e.code() == code;
    }
    return false;
}

struct UserTag {
    static constexpr std::string_view prefix = "user";
};
struct OrderTag {
    static constexpr std::string_view prefix = "order";
};
using UserID = tid::TypedID<UserTag>;
using OrderID = tid::TypedID<OrderTag>;

// Compile-time type safety: a UserID is not an OrderID.
static_assert(!std::is_convertible_v<UserID, OrderID>);
static_assert(!std::is_convertible_v<OrderID, UserID>);
static_assert(UserID::prefix() == "user");
static_assert(tid::detail::prefix_valid("my__type"));
static_assert(!tid::detail::prefix_valid("_user"));
static_assert(!tid::detail::prefix_valid("User"));
static_assert(!tid::detail::prefix_valid(std::string_view("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl")));

// Including the implementation must not leak min/max macros from <windows.h>.
static_assert(std::numeric_limits<int>::max() > 0);
static int min_max_still_work() { return std::max(1, 2) - std::min(1, 2); }

static void test_vectors() {
    for (const auto &v : VALID) {
        auto id = tid::TypeID::parse(v.str);
        CHECK(id.prefix() == v.prefix);
        CHECK(id.uuid() == v.uuid);
        CHECK(tid::TypeID::from_uuid(v.prefix, v.uuid).to_string() == v.str);
        CHECK(tid::TypeID::try_parse(v.str).has_value());
    }
    for (const auto &v : INVALID) {
        CHECK(!tid::TypeID::try_parse(v.str).has_value());
        bool threw = false;
        try {
            tid::TypeID::parse(v.str);
        } catch (const tid::Error &) {
            threw = true;
        }
        CHECK(threw);
    }
}

static void test_basic() {
    auto id = tid::TypeID::generate("user");
    CHECK(id.prefix() == "user");
    CHECK(id.to_string().size() == 31);
    CHECK(id.suffix().size() == 26);
    CHECK(tid::TypeID::parse(id.to_string()) == id);
    CHECK(!id.is_nil());
    CHECK(tid::TypeID().is_nil());
    CHECK(tid::TypeID::generate().prefix().empty());

    auto known = tid::TypeID::parse("prefix_01h455vb4pex5vsknk084sn02q");
    CHECK(known.uuid() == "01890a5d-ac96-774b-bcce-b302099a8057");
    CHECK(known.suffix() == "01h455vb4pex5vsknk084sn02q");
    CHECK(known.timestamp_ms() == 0x01890a5dac96ULL);
    CHECK(std::chrono::duration_cast<std::chrono::milliseconds>(
              known.timestamp().time_since_epoch())
              .count() == 0x01890a5dac96LL);
    CHECK(tid::TypeID::from_bytes("prefix", known.uuid_bytes()) == known);

    std::ostringstream os;
    os << known;
    CHECK(os.str() == "prefix_01h455vb4pex5vsknk084sn02q");
}

static void test_errors() {
    CHECK(throws_code([] { tid::TypeID::generate("User"); }, tid::Errc::invalid_prefix));
    CHECK(throws_code([] { tid::TypeID::generate(std::string(64, 'a')); },
                      tid::Errc::invalid_prefix));
    CHECK(throws_code([] { tid::TypeID::parse("user_8zzzzzzzzzzzzzzzzzzzzzzzzz"); },
                      tid::Errc::overflow));
    CHECK(throws_code([] { tid::TypeID::parse("user_123"); }, tid::Errc::invalid_suffix));
    CHECK(throws_code([] { tid::TypeID::from_uuid("user", "not-a-uuid"); },
                      tid::Errc::invalid_uuid));
    CHECK(throws_code(
        [] { tid::TypeID::parse("post_01h455vb4pex5vsknk084sn02q", "user"); },
        tid::Errc::prefix_mismatch));
    // parse_n semantics: string_view need not be NUL-terminated.
    std::string_view sv("user_01h455vb4pex5vsknk084sn02qXYZ", 31);
    CHECK(tid::TypeID::try_parse(sv).has_value());
}

static void test_typed() {
    UserID u = UserID::generate();
    CHECK(u.to_string().rfind("user_", 0) == 0);
    CHECK(UserID::parse(u.to_string()) == u);
    CHECK(throws_code([] { UserID::parse("order_01h455vb4pex5vsknk084sn02q"); },
                      tid::Errc::prefix_mismatch));
    CHECK(!UserID::try_parse("order_01h455vb4pex5vsknk084sn02q").has_value());
    CHECK(OrderID::try_parse("order_01h455vb4pex5vsknk084sn02q").has_value());
    CHECK(UserID().is_nil());
    CHECK(UserID().to_string() == "user_00000000000000000000000000");
    CHECK(UserID::from_uuid("01890a5d-ac96-774b-bcce-b302099a8057").to_string() ==
          "user_01h455vb4pex5vsknk084sn02q");
    CHECK(u.untyped().prefix() == "user");
}

static void test_generator() {
    tid::Generator gen;
    std::vector<std::string> ids;
    for (int i = 0; i < 50000; ++i) ids.push_back(gen.next("evt").to_string());
    CHECK(std::is_sorted(ids.begin(), ids.end()));
    CHECK(std::set<std::string>(ids.begin(), ids.end()).size() == ids.size());

    std::vector<OrderID> orders;
    for (int i = 0; i < 1000; ++i) orders.push_back(OrderID::generate(gen));
    CHECK(std::is_sorted(orders.begin(), orders.end()));

    // Shared generator across threads: all IDs unique.
    std::vector<std::vector<tid::TypeID>> per_thread(4);
    std::vector<std::thread> threads;
    for (auto &out : per_thread)
        threads.emplace_back([&gen, &out] {
            for (int i = 0; i < 5000; ++i) out.push_back(gen.next("job"));
        });
    for (auto &t : threads) t.join();
    std::unordered_set<tid::TypeID> all;
    for (auto &v : per_thread) {
        CHECK(std::is_sorted(v.begin(), v.end()));
        all.insert(v.begin(), v.end());
    }
    CHECK(all.size() == 20000);
}

static void test_hash_and_order() {
    auto a = tid::TypeID::parse("a_00000000000000000000000001");
    auto b = tid::TypeID::parse("a_00000000000000000000000002");
    auto c = tid::TypeID::parse("b_00000000000000000000000000");
    CHECK(a < b && b < c && a != b && a <= a && c >= b && c > a);
    std::hash<tid::TypeID> h;
    CHECK(h(a) == h(tid::TypeID::parse("a_00000000000000000000000001")));
    CHECK(h(a) != h(b));
    std::unordered_set<UserID> users{UserID::generate(), UserID::generate()};
    CHECK(users.size() == 2);
}

int main() {
    CHECK(min_max_still_work() == 1);
    test_vectors();
    test_basic();
    test_errors();
    test_typed();
    test_generator();
    test_hash_and_order();
    std::cout << checks << " checks, " << failures << " failures (C++)\n";
    return failures ? 1 : 0;
}
