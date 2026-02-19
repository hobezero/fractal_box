#include "fractal_box/runtime/message_manager.hpp"

#include <algorithm>
#include <array>
#include <variant>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std::string_view_literals;

namespace {

struct UpdateX { };
struct UpdateY { };

struct TtlPack {
	template<class T>
	constexpr
	auto of() const noexcept -> fr::MessageTtl {
		if constexpr (std::is_same_v<typename T::TickAt, UpdateX>)
			return this->x;
		else if constexpr (std::is_same_v<typename T::TickAt, UpdateY>)
			return this->y;
		else
			static_assert(false);
	}

public:
	fr::MessageTtl x;
	fr::MessageTtl y;
};

struct MsgA {
	using TickAt = UpdateX;
	static constexpr auto name = "A"sv;
	friend auto operator==(MsgA, MsgA) -> bool = default;

public:
	int x;
};

struct MsgB {
	using TickAt = UpdateY;
	static constexpr auto name = "B"sv;
	friend auto operator==(MsgB, MsgB) -> bool = default;

public:
	int y;
	char z;
};

struct MsgC {
	using TickAt = UpdateX;
	static constexpr auto name = "C"sv;
	friend auto operator==(MsgC, MsgC) -> bool = default;

public:
	double w;
};

using MsgVariant = std::variant<MsgA, MsgB, MsgC>;
using MsgPair = std::pair<fr::MessageTtl, MsgVariant>;

static constexpr MsgPair pushed[] = {
	{fr::MessageTtl{2}, MsgA{.x = 10}},
	{fr::MessageTtl{3}, MsgB{.y = 15, .z = 'q'}},
	{fr::MessageTtl::Persistent, MsgB{.y = 20, .z = 'w'}},
	{fr::MessageTtl{2}, MsgB{.y = 25, .z = 'e'}},
	{fr::MessageTtl{2}, MsgC{.w = 30.}},
	{fr::MessageTtl{2}, MsgB{.y = 35, .z = 'r'}},
	{fr::MessageTtl::Persistent, MsgC{.w = 35.}},
	{fr::MessageTtl{2}, MsgC{.w = 40.}},
	{fr::MessageTtl{1}, MsgA{.x = 45}},
	{fr::MessageTtl{2}, MsgA{.x = 50}},
	{fr::MessageTtl{3}, MsgC{.w = 55.}},
	{fr::MessageTtl{1}, MsgB{.y = 60, .z = 't'}},
	{fr::MessageTtl{1}, MsgB{.y = 65, .z = 'y'}},
	{fr::MessageTtl{4}, MsgA{.x = 70}},
	{fr::MessageTtl{3}, MsgC{.w = 75.}},
	{fr::MessageTtl{1}, MsgB{.y = 80, .z = 'y'}},
	{fr::MessageTtl{2}, MsgB{.y = 85, .z = 'u'}},
	{fr::MessageTtl::Persistent, MsgB{.y = 90, .z = 'i'}},
	{fr::MessageTtl::Persistent, MsgA{.x = 95}},
	{fr::MessageTtl::Persistent, MsgA{.x = 100}},
	{fr::MessageTtl::Persistent, MsgC{.w = 105.}},
	{fr::MessageTtl{3}, MsgC{.w = 110.}},
	{fr::MessageTtl{4}, MsgB{.y = 115, .z = 'o'}},
	{fr::MessageTtl{2}, MsgA{.x = 120}},
	{fr::MessageTtl{1}, MsgC{.w = 125.}},
	{fr::MessageTtl{4}, MsgB{.y = 130, .z = 'p'}},
	{fr::MessageTtl{3}, MsgB{.y = 135, .z = 'a'}},
};

template<class T>
constexpr
auto count_pushed(TtlPack min_ttls) noexcept -> size_t {
	const auto min_ttl = min_ttls.of<T>();
	return static_cast<size_t>(std::ranges::count_if(pushed, [min_ttl](const auto& pair) {
		return pair.first >= min_ttl && std::holds_alternative<T>(pair.second);
	}));
}

template<class... Ts>
static
void test_reader_for_each_empty(fr::MessageReader<Ts...>& reader) {
	INFO(fmt::format("expecting MessageReader<{}> to be empty",
		fmt::join(std::array{Ts::name...}, ", ")));

	CHECK_FALSE(reader.has_pending());
	([&] { CHECK_FALSE(reader.template has_pending_of<Ts>()); }(), ...);

	auto visited = std::vector<MsgVariant>{};
	const auto read_count = reader.for_each([&](const auto& msg) {
		visited.push_back(msg);
	});

	CHECK(read_count == 0);
	CHECK(visited == std::vector<MsgVariant>{}); // NOLINT
}

template<class... Ts, class ShouldConsume = fr::FalseC, class ExpectConsumed = fr::FalseC>
static
void test_reader_for_each(
	fr::MessageReader<Ts...>& reader,
	TtlPack min_ttls,
	ShouldConsume should_consume = {},
	ExpectConsumed expect_consumed = {}
) {
	INFO(fmt::format("testing MessageReader<{}>::for_each",
		fmt::join(std::array{Ts::name...}, ", ")));
	constexpr auto consume_period = 3uz;
	const auto expected = [&] {
		auto result = std::vector<MsgVariant>{};
		auto i = 0uz;
		for (const auto& [ttl, msg] : pushed) {
			if ((... || (std::holds_alternative<Ts>(msg) ? ttl >= min_ttls.of<Ts>() : false))) {
				if (!expect_consumed || i % consume_period != 0) {
					result.push_back(msg);
				}
				++i;
			}
		}
		return result;
	}();

	if (!expected.empty()) {
		CHECK(reader.has_pending());
		([&] {
			const auto expecting_t = std::ranges::find_if(expected, [](const auto& msg) {
				return std::holds_alternative<Ts>(msg);
			}) != expected.end();
			if (expecting_t) {
				CHECK(reader.template has_pending_of<Ts>());
			}
		}(), ...);
	}
	{
		auto visited = std::vector<MsgVariant>{};
		auto i = 0uz;
		const auto read_count = reader.for_each([&](const auto& msg) {
			visited.push_back(msg);
			if constexpr (should_consume) {
				return (i++ % consume_period) == 0;
			}
			++i;
		});
		CHECK(read_count == expected.size());
		CHECK(visited == expected);
	}

	INFO("after for_each");
	test_reader_for_each_empty(reader);
}

} // namespace

template<>
struct Catch::StringMaker<MsgA> {
	static auto convert(MsgA msg) -> std::string { return fmt::format("A({})", msg.x); }
};

template<>
struct Catch::StringMaker<MsgB> {
	static auto convert(MsgB msg) -> std::string { return fmt::format("B({}, {})", msg.y, msg.z); }
};

template<>
struct Catch::StringMaker<MsgC> {
	static auto convert(MsgC msg) -> std::string { return fmt::format("C({})", msg.w); }
};

TEST_CASE("MessageListReader", "[u][engine][message]") {
	CHECK(std::same_as<fr::MessageListReader<MsgA, MsgC>, fr::MessageReader<MsgA, MsgC>>);
	CHECK(std::same_as<
		fr::MessageListReader<fr::MpList<MsgA, MsgC>>,
		fr::MessageReader<MsgA, MsgC>
	>);
	CHECK(std::same_as<
		fr::MessageListReader<fr::MpList<MsgA, fr::MpList<MsgB>>, fr::MpList<MsgC>>,
		fr::MessageReader<MsgA, MsgB, MsgC>
	>);
}

TEST_CASE("MessageListWriter", "[u][engine][message]") {
	CHECK(std::same_as<fr::MessageListWriter<MsgA, MsgC>, fr::MessageWriter<MsgA, MsgC>>);
	CHECK(std::same_as<
		fr::MessageListWriter<fr::MpList<MsgA, MsgC>>,
		fr::MessageWriter<MsgA, MsgC>
	>);
	CHECK(std::same_as<
		fr::MessageListWriter<fr::MpList<MsgA, fr::MpList<MsgB>>, fr::MpList<MsgC>>,
		fr::MessageWriter<MsgA, MsgB, MsgC>
	>);
}

TEST_CASE("MessageManager", "[u][engine][message]") {
	fr::MessageManager manager;
	manager.register_tick_phases<UpdateX, UpdateY>();

	auto a_writer = manager.make_writer<MsgA>();
	auto b_writer = manager.make_writer<MsgB>();
	auto c_writer = manager.make_writer<MsgC>();
	for (const auto& [ttl, msg] : pushed) {
		std::visit([&]<class T>(const T& v) {
			if constexpr (std::is_same_v<T, MsgA>)
				a_writer.push(v, ttl);
			else if constexpr (std::is_same_v<T, MsgB>)
				b_writer.push(v, ttl);
			else if constexpr (std::is_same_v<T, MsgC>)
				c_writer.push(v, ttl);
			else
				static_assert(fr::always_false<T>);
		}, msg);
	}

	const auto test_reader = [&](TtlPack min_ttls) {
		const auto section = fmt::format("after {} UpdateX ticks and {} UpdateY ticks",
			min_ttls.x.value - 1, min_ttls.y.value - 1);
		SECTION(section) {
			REQUIRE(manager.get_queue_of<MsgA>().size() == count_pushed<MsgA>(min_ttls));
			REQUIRE(manager.get_queue_of<MsgB>().size() == count_pushed<MsgB>(min_ttls));
			REQUIRE(manager.get_queue_of<MsgC>().size() == count_pushed<MsgC>(min_ttls));

			SECTION("read using single-type readers") {
				SECTION("don't consume messages") {
					auto a1_reader = manager.make_reader<MsgA>();
					auto b1_reader = manager.make_reader<MsgB>();
					auto c1_reader = manager.make_reader<MsgC>();
					test_reader_for_each(a1_reader, min_ttls);
					test_reader_for_each(b1_reader, min_ttls);
					test_reader_for_each(c1_reader, min_ttls);

					auto a2_reader = manager.make_reader<MsgA>();
					auto b2_reader = manager.make_reader<MsgB>();
					auto c2_reader = manager.make_reader<MsgC>();
					test_reader_for_each(a2_reader, min_ttls);
					test_reader_for_each(b2_reader, min_ttls);
					test_reader_for_each(c2_reader, min_ttls);
				}
				SECTION("consume messages") {
					auto a1_reader = manager.make_reader<MsgA>();
					auto b1_reader = manager.make_reader<MsgB>();
					auto c1_reader = manager.make_reader<MsgC>();
					test_reader_for_each(a1_reader, min_ttls, fr::true_c);
					test_reader_for_each(b1_reader, min_ttls, fr::true_c);
					test_reader_for_each(c1_reader, min_ttls, fr::true_c);

					auto a2_reader = manager.make_reader<MsgA>();
					auto b2_reader = manager.make_reader<MsgB>();
					auto c2_reader = manager.make_reader<MsgC>();
					test_reader_for_each(a2_reader, min_ttls, fr::false_c, fr::true_c);
					test_reader_for_each(b2_reader, min_ttls, fr::false_c, fr::true_c);
					test_reader_for_each(c2_reader, min_ttls, fr::false_c, fr::true_c);

					a2_reader.consume_all();
					b2_reader.reset();
					c2_reader.consume_all_pending();
					INFO("after first consume_all()/reset()");
					test_reader_for_each_empty(a2_reader);
					test_reader_for_each(b2_reader, min_ttls, fr::false_c, fr::true_c);
					test_reader_for_each_empty(c2_reader);

					INFO("after second reset()");
					a2_reader.reset();
					test_reader_for_each_empty(a2_reader);
					c2_reader.reset();
					test_reader_for_each(c2_reader, min_ttls, fr::false_c, fr::true_c);
				}
			}
			SECTION("read using polymorphic readers") {
				SECTION("subset of message types") {
					SECTION("don't consume messages") {
						auto ab1_reader = manager.make_reader<MsgA, MsgB>();
						test_reader_for_each(ab1_reader, min_ttls);

						auto ab2_reader = manager.make_reader<MsgA, MsgB>();
						test_reader_for_each(ab2_reader, min_ttls);
					}
					SECTION("consume messages") {
						auto ab1_reader = manager.make_reader<MsgA, MsgB>();
						test_reader_for_each(ab1_reader, min_ttls, fr::true_c);

						auto ab2_reader = manager.make_reader<MsgA, MsgB>();
						test_reader_for_each(ab2_reader, min_ttls, fr::false_c, fr::true_c);

						ab2_reader.reset();
						INFO("after reset()");
						test_reader_for_each(ab2_reader, min_ttls, fr::false_c, fr::true_c);

						ab2_reader.consume_all_pending();
						INFO("after consume_all_pending()");
						test_reader_for_each_empty(ab2_reader);
					}
				}
				SECTION("every message type") {
					SECTION("don't consume messages") {
						auto abc1_reader = manager.make_reader<MsgA, MsgB, MsgC>();
						test_reader_for_each(abc1_reader, min_ttls);

						auto abc2_reader = manager.make_reader<MsgA, MsgB, MsgC>();
						test_reader_for_each(abc2_reader, min_ttls);
					}
					SECTION("consume messages") {
						auto abc1_reader = manager.make_reader<MsgA, MsgB, MsgC>();
						test_reader_for_each(abc1_reader, min_ttls, fr::true_c);

						auto abc2_reader = manager.make_reader<MsgA, MsgB, MsgC>();
						test_reader_for_each(abc2_reader, min_ttls, fr::false_c, fr::true_c);

						abc2_reader.reset();
						INFO("after reset()");
						test_reader_for_each(abc2_reader, min_ttls, fr::false_c, fr::true_c);

						abc2_reader.consume_all_pending();
						INFO("after consume_all_pending()");
						test_reader_for_each_empty(abc2_reader);
					}
				}
			}
		}
	};

	test_reader({.x = fr::MessageTtl{1}, .y = fr::MessageTtl{1}});

	manager.tick(fr::TickPhaseTypeIdx::of<UpdateX>);
	test_reader({.x = fr::MessageTtl{2}, .y = fr::MessageTtl{1}});

	manager.tick<UpdateX>(1);
	test_reader({.x = fr::MessageTtl{3}, .y = fr::MessageTtl{1}});

	manager.tick<UpdateY>();
	test_reader({.x = fr::MessageTtl{3}, .y = fr::MessageTtl{2}});

	manager.tick<UpdateY>(2);
	test_reader({.x = fr::MessageTtl{3}, .y = fr::MessageTtl{4}});

	manager.tick<UpdateX>();
	test_reader({.x = fr::MessageTtl{4}, .y = fr::MessageTtl{4}});
}
