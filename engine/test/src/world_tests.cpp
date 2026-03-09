#include "fractal_box/runtime/world.hpp"

#include <memory>
#include <random>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"

#include "test_common/test_helpers.hpp"

namespace {

FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_UNNEEDED_INTERNAL
FR_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION

struct Pos {
	[[maybe_unused]] friend
	auto operator==(const Pos&, const Pos&) -> bool = default;

	friend
	auto fr_custom_is_trivially_relocatable(Pos) -> fr::TrueC;

public:
	float x;
	float y;
};

struct Velocity {
	[[maybe_unused]] friend
	auto operator==(const Velocity&, const Velocity&) -> bool = default;

	friend
	auto fr_custom_is_trivially_relocatable(Velocity) -> fr::TrueC;

public:
	float mps;
};

struct MyStr {
	[[maybe_unused]] friend
	auto operator==(const MyStr&, const MyStr&) -> bool = default;

public:
	std::string data;
};

class MyBuffer {
public:
	MyBuffer() = default;

	explicit
	MyBuffer(size_t size): _data{std::make_unique<int[]>(size)}, _size{size} { }

	explicit
	MyBuffer(size_t size, int value):
		_data{std::make_unique_for_overwrite<int[]>(size)},
		_size{size}
	{
		std::ranges::uninitialized_fill_n(_data.get(), static_cast<ptrdiff_t>(size), value);
	}

	auto operator==(const MyBuffer& other) const noexcept -> bool {
		return std::ranges::equal(data(), other.data());
	}

	friend
	auto kepler_custom_is_trivially_relocatable(MyBuffer) -> fr::FalseC;

	auto data() const noexcept -> std::span<const int> { return {_data.get(), _size.value()}; }
	auto data() noexcept -> std::span<int> { return {_data.get(), _size.value()}; }
	auto size() const noexcept -> size_t { return _size.value(); }

private:
	std::unique_ptr<int[]> _data;
	fr::WithDefault<size_t, 0zu> _size;
};

/// @brief Same as MyBuffer, except marked relocatable
class MyData {
public:
	MyData() = default;

	explicit
	MyData(size_t size): _data{std::make_unique<int[]>(size)}, _size{size} { }

	explicit
	MyData(size_t size, int value):
		_data{std::make_unique_for_overwrite<int[]>(size)},
		_size{size}
	{
		std::ranges::uninitialized_fill_n(_data.get(), static_cast<ptrdiff_t>(size), value);
	}

	auto operator==(const MyData& other) const noexcept -> bool {
		return std::ranges::equal(data(), other.data());
	}

	friend
	auto kepler_custom_is_trivially_relocatable(MyData) -> fr::TrueC;

	auto data() const noexcept -> std::span<const int> { return {_data.get(), _size.value()}; }
	auto data() noexcept -> std::span<int> { return {_data.get(), _size.value()}; }
	auto size() const noexcept -> size_t { return _size.value(); }

private:
	std::unique_ptr<int[]> _data;
	fr::WithDefault<size_t, 0zu> _size;
};

struct MyNum {
	[[maybe_unused]] friend
	auto operator==(MyNum, MyNum) -> bool = default;

public:
	size_t value;
};

struct MyChar {
	[[maybe_unused]] friend
	auto operator==(MyChar, MyChar) -> bool = default;

public:
	char value;
};

struct alignas(2 * sizeof(uint64_t)) MySimd {
	[[maybe_unused]] friend
	auto operator==(MySimd, MySimd) -> bool = default;

public:
	uint64_t lo;
	uint64_t hi;
};

struct EnemyTag { };
struct PlayerTag { };

} // namespace

template<>
struct fmt::formatter<Pos>: formatter<char> {
	static
	auto format(const Pos& pos, format_context& ctx) {
		return fmt::format_to(ctx.out(), "({}, {})", pos.x, pos.y);
	}
};

template<>
struct fmt::formatter<Velocity>: formatter<float> {
	static
	auto format(const Velocity& velocity, format_context& ctx) {
		return fmt::format_to(ctx.out(), "{}", velocity.mps);
	}
};

template<>
struct fmt::formatter<MyStr>: formatter<char> {
	static
	auto format(const MyStr& str, format_context& ctx) {
		return fmt::format_to(ctx.out(), "{}", str.data);
	}
};

template<>
struct fmt::formatter<MyData>: formatter<char> {
	static
	auto format(const MyData& data, format_context& ctx) {
		if (data.data().empty())
			return fmt::format_to(ctx.out(), "{{}}");

		return fmt::format_to(ctx.out(), "{{{}[{}]}}", data.data()[0], data.size());
	}
};

template<>
struct fmt::formatter<MyNum>: formatter<char> {
	static
	auto format(const MyNum& num, format_context& ctx) {
		return fmt::format_to(ctx.out(), "{}", num.value);
	}
};

template<>
struct fmt::formatter<MyChar>: formatter<char> {
	static
	auto format(const MyChar& ch, format_context& ctx) {
		return fmt::format_to(ctx.out(), "{}", ch.value);
	}
};

template<>
struct fmt::formatter<MySimd>: formatter<char> {
	static
	auto format(const MySimd& simd, format_context& ctx) {
		return fmt::format_to(ctx.out(), "{{{}|{}}}", simd.lo, simd.hi);
	}
};

FR_DIAGNOSTIC_POP

#define FR_TEST_DEFINE_CATCH_STRING_MAKER(Type) \
	template<> \
	struct Catch::StringMaker<Type> { \
		static \
		auto convert(fr::PassAbi<Type> obj) -> std::string { return fmt::format("{}", obj); } \
	};

FR_TEST_DEFINE_CATCH_STRING_MAKER(Pos)
FR_TEST_DEFINE_CATCH_STRING_MAKER(Velocity)
FR_TEST_DEFINE_CATCH_STRING_MAKER(MyStr)
FR_TEST_DEFINE_CATCH_STRING_MAKER(MyData)
FR_TEST_DEFINE_CATCH_STRING_MAKER(MyNum)
FR_TEST_DEFINE_CATCH_STRING_MAKER(MyChar)
FR_TEST_DEFINE_CATCH_STRING_MAKER(MySimd)

TEST_CASE("Arhetype.hash", "[u][engine][runtime][ecs][world]") {
	using Pair = std::pair<unsigned, unsigned>;
	using Triplet = std::tuple<unsigned, unsigned, unsigned>;

	const auto hasher = fr::WorldHasher{};

	SECTION("no component has a zero hash code") {
		for (auto a_val = 0u; a_val < 2048u; ++a_val) {
			const auto a = fr::ComponentTypeIdx{fr::adopt, a_val};
			CHECK(hasher(a) != fr::Archetype<fr::EntityId32>::neutral_hash);
		}
	}
	SECTION("no component's hash collides with another component") {
		auto collisions = std::vector<Pair>{};
		static constexpr auto limit = 2048u;
		for (auto a_val = 0u; a_val < limit; ++a_val) {
			for (auto b_val = a_val + 1u; b_val < limit; ++b_val) {
				const auto a = fr::ComponentTypeIdx{fr::adopt, a_val};
				const auto b = fr::ComponentTypeIdx{fr::adopt, b_val};

				if (hasher(a) == hasher(b))
					collisions.emplace_back(a_val, b_val);
			}
		}
		CHECK(collisions == std::vector<Pair>{});
	}
	SECTION("no component's hash collides with two other components") {
		auto collisions = std::vector<Triplet>{};
		static constexpr auto limit = 256u;
		for (auto a_val = 0u; a_val < limit; ++a_val) {
			for (auto b_val = a_val + 1u; b_val < limit; ++b_val) {
				for (auto c_val = b_val + 1u; c_val < limit; ++c_val) {
					const auto a = fr::ComponentTypeIdx{fr::adopt, a_val};
					const auto b = fr::ComponentTypeIdx{fr::adopt, b_val};
					const auto c = fr::ComponentTypeIdx{fr::adopt, c_val};

					if (hasher(a) == fr::Archetype<fr::EntityId32>::hash_add(hasher(b), hasher(c)))
						collisions.emplace_back(a_val, b_val, c_val);
					if (hasher(b) == fr::Archetype<fr::EntityId32>::hash_add(hasher(a), hasher(c)))
						collisions.emplace_back(b_val, a_val, c_val);
					if (hasher(c) == fr::Archetype<fr::EntityId32>::hash_add(hasher(a), hasher(b)))
						collisions.emplace_back(c_val, a_val, b_val);
				}
			}
		}
		CHECK(collisions == std::vector<Triplet>{});
	}
}

template<fr::c_world TWorld, class Comp>
static
void check_entity_has_component_value(
	const TWorld& world,
	typename TWorld::Entity eid,
	const Comp& comp
) {
	INFO(fmt::format("expecting component `{}` with value '{}'", fr::type_name<Comp>, comp));
	REQUIRE(world.template has_component<Comp>(eid));
	REQUIRE(world.template try_get_component<Comp>(eid));
	CHECK(*world.template try_get_component<Comp>(eid) == comp);
	CHECK(world.template get_component<Comp>(eid) == comp);
}

TEST_CASE("World.type-properties", "[u][engine][runtime][ecs][world]") {
	STATIC_CHECK(std::is_default_constructible_v<fr::World<>>);

	STATIC_CHECK_FALSE(std::is_copy_constructible_v<fr::World<>>);
	STATIC_CHECK_FALSE(std::is_copy_assignable_v<fr::World<>>);

	STATIC_CHECK(std::is_move_constructible_v<fr::World<>>);
	STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::World<>>);

	STATIC_CHECK(std::is_move_assignable_v<fr::World<>>);
	STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::World<>>);

	STATIC_CHECK(std::is_destructible_v<fr::World<>>);
	STATIC_CHECK(std::is_nothrow_destructible_v<fr::World<>>);
}

TEST_CASE("World.emplace_components", "[u][engine][runtime][ecs][world]") {
	const auto a_pos = Pos{1.f, 2.f};
	const auto a_velocity = Velocity{30.f};
	const auto a_str = MyStr{"abc"};
	const auto a_num = MyNum{234};
	const auto a_data_size = 40;
	const auto a_data_val = -23;
	const auto a_data = MyData{a_data_size, a_data_val};
	const auto a_simd = MySimd{456, 789};

	auto world = fr::World<>{};
	const auto a = world.spawn();
	const auto b = world.spawn();

	{
		INFO("add component to a dead entity");
		world.despawn(b);
		auto res = world.emplace_components(b, fr::in_place_args<Pos>(a_pos.x, a_pos.y));
		CHECK(res.inserted_count() == 0);
		CHECK_FALSE(res.inserted_all());
		CHECK(res.get<Pos>() == nullptr);
	}
	{
		INFO("add one component");
		CHECK_FALSE(world.has_component<Pos>(a));
		auto res = world.emplace_components(a, fr::in_place_args<Pos>(a_pos.x, a_pos.y));
		CHECK(res.inserted_all());
		CHECK(res.inserted_count() == 1);
		CHECK(res.was_inserted());
		CHECK(res.was_inserted<Pos>());
		CHECK(res.was_inserted<0>());
		CHECK(res.was_inserted(0));
		REQUIRE(res.get<0>());
		CHECK(*res.get<0>() == a_pos);
		REQUIRE(res.get<Pos>());
		CHECK(*res.get<Pos>() == a_pos);
		check_entity_has_component_value(world, a, a_pos);
	}
	{
		INFO("add two components");
		auto [vel, str] = world.emplace_components(a,
			a_velocity,
			fr::in_place_args<MyStr>(a_str.data)
		);
		REQUIRE(vel);
		CHECK(*vel == a_velocity);
		REQUIRE(str);
		CHECK(*str == a_str);
		check_entity_has_component_value(world, a, a_pos);
		check_entity_has_component_value(world, a, a_velocity);
		check_entity_has_component_value(world, a, a_str);
	}
	{
		INFO("add four components");
		auto res = world.emplace_components(a,
			fr::in_place_args<MySimd>(a_simd.lo, a_simd.hi),
			a_num,
			fr::in_place_args<Velocity>(a_velocity),
			fr::in_place_args<MyData>(a_data_size, a_data_val)
		);
		CHECK(res.inserted_count() == 3);
		CHECK(res.was_inserted<MySimd>());
		CHECK(res.was_inserted<MyNum>());
		CHECK_FALSE(res.was_inserted<Velocity>());
		CHECK(res.was_inserted<MyData>());
		REQUIRE(res.get<0>());
		REQUIRE(res.get<1>());
		REQUIRE(res.get<2>());
		REQUIRE(res.get<3>());
		REQUIRE(res.get<MySimd>());
		REQUIRE(res.get<MyNum>());
		REQUIRE(res.get<Velocity>());
		REQUIRE(res.get<MyData>());
		CHECK(*res.get<0>() == a_simd);
		CHECK(*res.get<1>() == a_num);
		CHECK(*res.get<2>() == a_velocity);
		CHECK(*res.get<3>() == a_data);
		CHECK(*res.get<MySimd>() == a_simd);
		CHECK(*res.get<MyNum>() == a_num);
		CHECK(*res.get<Velocity>() == a_velocity);
		CHECK(*res.get<MyData>() == a_data);

		check_entity_has_component_value(world, a, a_pos);
		check_entity_has_component_value(world, a, a_velocity);
		check_entity_has_component_value(world, a, a_str);
		check_entity_has_component_value(world, a, a_simd);
		check_entity_has_component_value(world, a, a_data);
	}
}

TEST_CASE("World.spawn_with", "[u][engine][runtime][ecs][world]") {
	auto world = fr::World<>{};

	const auto pos = Pos{2.4f, 3.6f};
	const auto str = MyStr{frt::lorem_text};
	const auto velocity = Velocity{67.f};

	auto a = world.spawn_with(
		fr::in_place_args<Pos>(pos.x, pos.y),
		fr::in_place_args<MyStr>(str.data),
		velocity
	);

	check_entity_has_component_value(world, a, pos);
	check_entity_has_component_value(world, a, str);
	check_entity_has_component_value(world, a, velocity);
}

TEST_CASE("World.remove_components", "[u][engine][runtime][ecs][world]") {
	auto world = fr::World<>{};

	const auto pos = Pos{2.4f, 3.6f};
	const auto str = MyStr{frt::lorem_text};
	const auto velocitty = Velocity{67.f};
	const auto data_val = 234;
	const auto data_size = 7zu;
	const auto data = MyData{data_size, data_val};
	const auto simd = MySimd{345, 678};
	const auto ch = MyChar{'a'};
	const auto num = MyNum{5};

	auto a = world.spawn();

	INFO("emplace eight components");
	world.emplace_component<Pos>(a, pos);
	world.emplace_component<Velocity>(a, velocitty.mps);
	world.emplace_component<MyStr>(a, str.data);
	world.emplace_component<MyData>(a, data_size, data_val);
	world.emplace_component<MySimd>(a, simd);
	world.emplace_component<MyChar>(a, ch);
	world.emplace_component<MyNum>(a, num);
	world.emplace_component<PlayerTag>(a, num);
	CHECK(world.component_count(a) == 8);

	SECTION("remove three components") {
		CHECK(world.remove_components<MySimd, Pos, MyStr>(a) == 3);
		CHECK(world.component_count(a) == 5);
		CHECK_FALSE(world.has_component<Pos>(a));
		CHECK_FALSE(world.has_component<MyStr>(a));
		CHECK_FALSE(world.has_component<MySimd>(a));
		check_entity_has_component_value(world, a, velocitty);
		check_entity_has_component_value(world, a, data);
		check_entity_has_component_value(world, a, ch);
		check_entity_has_component_value(world, a, num);

		SECTION("remove one component (+ one already removed)") {
			CHECK(world.remove_components<MyStr, MyChar>(a) == 1);
			CHECK(world.component_count(a) == 4);
			CHECK_FALSE(world.has_component<Pos>(a));
			CHECK_FALSE(world.has_component<MyStr>(a));
			CHECK_FALSE(world.has_component<MySimd>(a));
			CHECK_FALSE(world.has_component<MyChar>(a));
			check_entity_has_component_value(world, a, velocitty);
			check_entity_has_component_value(world, a, data);
			check_entity_has_component_value(world, a, num);

			INFO("remove three components (+ two already removed)");
			CHECK(world.remove_components<MyNum, MyStr, Velocity, PlayerTag, Pos>(a) == 3);
			CHECK(world.component_count(a) == 1);
			CHECK_FALSE(world.has_component<Pos>(a));
			CHECK_FALSE(world.has_component<MyStr>(a));
			CHECK_FALSE(world.has_component<MySimd>(a));
			CHECK_FALSE(world.has_component<MyChar>(a));
			CHECK_FALSE(world.has_component<MyNum>(a));
			CHECK_FALSE(world.has_component<Velocity>(a));
			check_entity_has_component_value(world, a, data);

			INFO("remove one");
			CHECK(world.remove_components<MyData>(a) == 1);
			CHECK(world.component_count(a) == 0);
			CHECK_FALSE(world.has_component<Pos>(a));
			CHECK_FALSE(world.has_component<MyStr>(a));
			CHECK_FALSE(world.has_component<MySimd>(a));
			CHECK_FALSE(world.has_component<MyChar>(a));
			CHECK_FALSE(world.has_component<MyNum>(a));
			CHECK_FALSE(world.has_component<Velocity>(a));
			CHECK_FALSE(world.has_component<MyData>(a));
		}
		SECTION("remove five components (+ one nonexistent)") {
			CHECK(world.remove_components<MyChar, Velocity, MyData, MyBuffer, PlayerTag, MyNum>(a)
				== 5);
			CHECK(world.component_count(a) == 0);
			CHECK_FALSE(world.has_component<Pos>(a));
			CHECK_FALSE(world.has_component<Velocity>(a));
			CHECK_FALSE(world.has_component<MyStr>(a));
			CHECK_FALSE(world.has_component<MyData>(a));
			CHECK_FALSE(world.has_component<MySimd>(a));
			CHECK_FALSE(world.has_component<MyChar>(a));
			CHECK_FALSE(world.has_component<MyNum>(a));

			INFO("fail to remove already removed");
			CHECK(world.remove_components<Velocity, MyData>(a) == 0);
			CHECK(world.component_count(a) == 0);

			CHECK(world.remove_components<MyChar>(a) == 0);
			CHECK(world.component_count(a) == 0);
		}
	}
	SECTION("despawn, then remove components") {
		world.despawn(a);
		CHECK(world.remove_components<MyChar, MyNum>(a) == 0);
		CHECK_FALSE(world.contains(a));
	}
}

TEST_CASE("World.get_component", "[u][engine][runtime][ecs][world]") {
	const auto a_pos = Pos{.x = 20.f, .y = 30.f};
	const auto a_velocity = Velocity{45.f};
	const auto a_simd = MySimd{120, 130};

	auto world = fr::World<>{};
	auto a = world.spawn();

	CHECK(world.try_get_component<Pos>(a) == nullptr);

	world.emplace_component<Pos>(a, a_pos);
	REQUIRE(world.try_get_component<Pos>(a) != nullptr);
	CHECK(*world.try_get_component<Pos>(a) == a_pos);

	world.emplace_component<Velocity>(a, a_velocity);
	world.emplace_component<MySimd>(a, a_simd);
	world.emplace_component<PlayerTag>(a);
	auto [pos, vel, str, simd, player] = world.try_get_components<Pos, Velocity, MyStr, MySimd,
		PlayerTag>(a);
	REQUIRE(pos);
	REQUIRE(vel);
	REQUIRE(simd);
	REQUIRE(player);
	CHECK(*pos == a_pos);
	CHECK(*vel == a_velocity);
	CHECK(*simd == a_simd);
	CHECK(str == nullptr);

	world.remove_all_components(a);
	CHECK(world.try_get_component<Pos>(a) == nullptr);
}

TEST_CASE("World.component-operations", "[u][engine][runtime][ecs][world]") {
	auto world = fr::World<>{};

	const auto a_pos_value = Pos{.x = 20.f, .y = 30.f};
	const auto a_str_value = MyStr{"aksjflkasjfdklajsdlkfjalksfjlkasdjfaksjfdklj3i"};

	const auto b_pos_value = Pos{.x = 40.f, .y = 50.f};

	const auto c_pos_value = Pos{.x = 60., .y = 70.f};
	const auto c_velocity_value = Velocity{.mps = 3.f};
	const auto c_str_value = MyStr{"c test 32 awojdfh asjdf hdfkj hasdjfh hdfiuyqwf 34"};

	auto a = world.spawn();
	auto b = world.spawn();
	{
		INFO("add two components to 'a' and one to 'b'");

		CHECK_FALSE(world.has_component<Pos>(a));

		auto* a_pos = world.emplace_component<Pos>(a, a_pos_value).get<0>();
		CHECK_FALSE(world.has_component<Pos>(b));
		CHECK(world.entity_count_with_component<Pos>() == 1);
		CHECK(world.entity_count_with_component<MyStr>() == 0);

		auto* b_pos = world.emplace_component<Pos>(b, b_pos_value.x, b_pos_value.y).get<0>();
		check_entity_has_component_value(world, a, a_pos_value);
		check_entity_has_component_value(world, b, b_pos_value);
		CHECK(world.entity_count_with_component<Pos>() == 2);
		CHECK(world.entity_count_with_component<MyStr>() == 0);
		CHECK(&world.get_component<Pos>(a) == a_pos);
		CHECK(&world.get_component<Pos>(b) == b_pos);
		CHECK(a_pos != b_pos);

		CHECK_FALSE(world.has_component<MyStr>(a));
		world.emplace_component<MyStr>(a, a_str_value);
		check_entity_has_component_value(world, a, a_pos_value);
		check_entity_has_component_value(world, a, a_str_value);
		CHECK(world.entity_count_with_component<Pos>() == 2);
		CHECK(world.entity_count_with_component<MyStr>() == 1);
	}

	auto c = world.spawn();
	{
		INFO("add three components to 'c'");

		world.add_component(c, c_pos_value);
		world.emplace_component<MyStr>(c, c_str_value);
		world.emplace_component<Velocity>(c, c_velocity_value);
		check_entity_has_component_value(world, c, c_pos_value);
		check_entity_has_component_value(world, c, c_str_value);
		check_entity_has_component_value(world, c, c_velocity_value);
		CHECK(world.entity_count_with_component<Pos>() == 3);
		CHECK(world.entity_count_with_component<MyStr>() == 2);
		CHECK(world.entity_count_with_component<Velocity>() == 1);
	}

	SECTION("remove components one by one") {
		SECTION("remove MyStr first") {
			CHECK(world.remove_component<MyStr>(c));
			CHECK_FALSE(world.has_component<MyStr>(c));

			check_entity_has_component_value(world, c, c_pos_value);
			check_entity_has_component_value(world, c, c_velocity_value);
			CHECK(world.entity_count_with_component<Pos>() == 3);
			CHECK(world.entity_count_with_component<MyStr>() == 1);
			CHECK(world.entity_count_with_component<Velocity>() == 1);

		}
		SECTION("remove Pos first") {
			CHECK(world.remove_component<Pos>(c));
			CHECK_FALSE(world.has_component<Pos>(c));
			check_entity_has_component_value(world, c, c_str_value);
			check_entity_has_component_value(world, c, c_velocity_value);
			CHECK(world.entity_count_with_component<Pos>() == 2);
			CHECK(world.entity_count_with_component<MyStr>() == 2);
			CHECK(world.entity_count_with_component<Velocity>() == 1);

			CHECK(world.remove_component<MyStr>(c));
			CHECK_FALSE(world.has_component<MyStr>(c));
			check_entity_has_component_value(world, c, c_velocity_value);
			CHECK(world.entity_count_with_component<Pos>() == 2);
			CHECK(world.entity_count_with_component<MyStr>() == 1);
			CHECK(world.entity_count_with_component<Velocity>() == 1);

			CHECK_FALSE(world.remove_component<MyStr>(c));
			CHECK_FALSE(world.remove_component<Pos>(c));

			CHECK(world.remove_component<Velocity>(c));
			CHECK_FALSE(world.has_component<Velocity>(c));
			CHECK_FALSE(world.has_component<MyStr>(c));
			CHECK_FALSE(world.has_component<Pos>(c));
			CHECK(world.count_entities_in_tables() == 3);
			CHECK(world.entity_count() == 3);
			CHECK(world.entity_count_with_component<Pos>() == 2);
			CHECK(world.entity_count_with_component<MyStr>() == 1);
			CHECK(world.entity_count_with_component<Velocity>() == 0);
		}
		SECTION("remove Velocity first") {
			CHECK(world.remove_component<Velocity>(c));
			CHECK_FALSE(world.has_component<Velocity>(c));
			check_entity_has_component_value(world, c, c_pos_value);
			check_entity_has_component_value(world, c, c_str_value);
			CHECK(world.entity_count_with_component<Pos>() == 3);
			CHECK(world.entity_count_with_component<MyStr>() == 2);
			CHECK(world.entity_count_with_component<Velocity>() == 0);

			CHECK(world.remove_component<Pos>(c));
			CHECK_FALSE(world.has_component<Pos>(c));
			check_entity_has_component_value(world, c, c_str_value);
			CHECK(world.count_entities_in_tables() == 3);
			CHECK(world.entity_count() == 3);
			CHECK(world.entity_count_with_component<Pos>() == 2);
			CHECK(world.entity_count_with_component<MyStr>() == 2);
			CHECK(world.entity_count_with_component<Velocity>() == 0);

			CHECK(world.remove_component<MyStr>(c));
			CHECK_FALSE(world.has_component<MyStr>(c));
			CHECK_FALSE(world.has_component<Velocity>(c));
			CHECK_FALSE(world.has_component<Pos>(c));
			CHECK(world.count_entities_in_tables() == 3);
			CHECK(world.entity_count() == 3);
			CHECK(world.entity_count_with_component<Pos>() == 2);
			CHECK(world.entity_count_with_component<MyStr>() == 1);
			CHECK(world.entity_count_with_component<Velocity>() == 0);
		}
	}
	SECTION("remove all components") {
		CHECK(world.remove_all_components(c));
		CHECK_FALSE(world.has_component<MyStr>(c));
		CHECK_FALSE(world.has_component<Velocity>(c));
		CHECK_FALSE(world.has_component<Pos>(c));
		CHECK(world.count_entities_in_tables() == 3);
		CHECK(world.entity_count() == 3);
		CHECK(world.component_count(c) == 0);
		CHECK(world.entity_count_with_component<Pos>() == 2);
		CHECK(world.entity_count_with_component<MyStr>() == 1);
		CHECK(world.entity_count_with_component<Velocity>() == 0);
	}
	SECTION("destroy entity") {
		world.despawn(b);
		CHECK(world.contains(a));
		CHECK_FALSE(world.contains(b));
		CHECK(world.contains(c));
		CHECK(world.count_entities_in_tables() == 2);
		CHECK(world.entity_count() == 2);
		CHECK(world.entity_count_with_component<Pos>() == 2);
		CHECK(world.entity_count_with_component<MyStr>() == 2);
		CHECK(world.entity_count_with_component<Velocity>() == 1);

		world.remove_all_components(c);
		world.despawn(c);
		CHECK(world.contains(a));
		CHECK_FALSE(world.contains(b));
		CHECK_FALSE(world.contains(c));
		CHECK(world.count_entities_in_tables() == 1);
		CHECK(world.entity_count() == 1);
		CHECK(world.entity_count_with_component<Pos>() == 1);
		CHECK(world.entity_count_with_component<MyStr>() == 1);
		CHECK(world.entity_count_with_component<Velocity>() == 0);

		const auto d = world.spawn();
		CHECK(world.contains(a));
		CHECK_FALSE(world.contains(b));
		CHECK_FALSE(world.contains(c));
		CHECK(world.contains(d));
		CHECK(world.count_entities_in_tables() == 2);
		CHECK(world.entity_count() == 2);

		const auto e = world.spawn();
		CHECK(world.contains(a));
		CHECK_FALSE(world.contains(b));
		CHECK_FALSE(world.contains(c));
		CHECK(world.contains(d));
		CHECK(world.contains(e));
		CHECK(world.count_entities_in_tables() == 3);
		CHECK(world.entity_count() == 3);

		world.despawn(e);
		CHECK(world.contains(a));
		CHECK_FALSE(world.contains(b));
		CHECK_FALSE(world.contains(c));
		CHECK(world.contains(d));
		CHECK_FALSE(world.contains(e));
		CHECK(world.count_entities_in_tables() == 2);
		CHECK(world.entity_count() == 2);

		const auto f = world.spawn();
		CHECK(world.contains(a));
		CHECK_FALSE(world.contains(b));
		CHECK_FALSE(world.contains(c));
		CHECK(world.contains(d));
		CHECK_FALSE(world.contains(e));
		CHECK(world.contains(f));
		CHECK(world.count_entities_in_tables() == 3);
		CHECK(world.entity_count() == 3);
	}
}

TEST_CASE("World.tag-components", "[u][engine][runtime][ecs][world]") {
	const auto a_pos = Pos{2.3f, 4.5f};
	const auto a_vel = Velocity{20.f};
	const auto a_num = MyNum{23};

	auto world = fr::World<>{};
	const auto a = world.spawn();
	CHECK_FALSE(world.has_component<PlayerTag>(a));

	INFO("emplace_component(PlayerTag, Velocity)");
	CHECK(world.emplace_component<PlayerTag>(a).inserted_all());
	CHECK(world.emplace_component<Velocity>(a, a_vel).inserted_all());
	CHECK(world.has_component<PlayerTag>(a));

	INFO("fail to emplace_component(PlayerTag)");
	CHECK_FALSE(world.emplace_component<PlayerTag>(a).was_inserted());

	SECTION("emplace_component(Pos, EnemyTag)") {
		CHECK(world.emplace_component<Pos>(a, a_pos).inserted_all());
		CHECK(world.emplace_component<EnemyTag>(a).inserted_all());
		check_entity_has_component_value(world, a, a_pos);
		CHECK(world.has_component<EnemyTag>(a));
	}
	SECTION("emplace_components(Pos, EnemyTag)") {
		CHECK(world.emplace_components(a, a_pos, EnemyTag{}).inserted_all());
		check_entity_has_component_value(world, a, a_pos);
		CHECK(world.has_component<EnemyTag>(a));

		SECTION("remove_component(Velocity, EnemyTag)") {
			CHECK(world.remove_component<Velocity>(a));
			CHECK(world.remove_component<EnemyTag>(a));
			CHECK(world.has_component<PlayerTag>(a));
			check_entity_has_component_value(world, a, a_pos);
			CHECK_FALSE(world.has_component<Velocity>(a));
			CHECK_FALSE(world.has_component<EnemyTag>(a));

			INFO("remove_component(Pos, PlayerTag)");
			world.remove_component<Pos>(a);
			world.remove_component<PlayerTag>(a);
			CHECK_FALSE(world.has_component<Pos>(a));
			CHECK_FALSE(world.has_component<Velocity>(a));
			CHECK_FALSE(world.has_component<PlayerTag>(a));
			CHECK_FALSE(world.has_component<EnemyTag>(a));
		}
		SECTION("remove_components(Velocity, EnemyTag)") {
			world.remove_components<Velocity, EnemyTag>(a);
			CHECK(world.has_component<PlayerTag>(a));
			check_entity_has_component_value(world, a, a_pos);
			CHECK_FALSE(world.has_component<Velocity>(a));
			CHECK_FALSE(world.has_component<EnemyTag>(a));

			INFO("remove_components(Pos, PlayerTag)");
			world.remove_components<Pos, PlayerTag>(a);
			CHECK_FALSE(world.has_component<Pos>(a));
			CHECK_FALSE(world.has_component<Velocity>(a));
			CHECK_FALSE(world.has_component<PlayerTag>(a));
			CHECK_FALSE(world.has_component<EnemyTag>(a));
		}
		SECTION("remove_components(PlayerTag, EnemyTag)") {
			world.remove_components<PlayerTag, EnemyTag>(a);
			check_entity_has_component_value(world, a, a_pos);
			check_entity_has_component_value(world, a, a_vel);
			CHECK_FALSE(world.has_component<PlayerTag>(a));
			CHECK_FALSE(world.has_component<EnemyTag>(a));
		}
		SECTION("remove_all_components") {
			world.remove_all_components(a);
			CHECK_FALSE(world.has_component<Pos>(a));
			CHECK_FALSE(world.has_component<Velocity>(a));
			CHECK_FALSE(world.has_component<PlayerTag>(a));
			CHECK_FALSE(world.has_component<EnemyTag>(a));
		}
	}
	SECTION("emplace_components(EnemyTag, PlayerTag, MyNum") {
		const auto res = world.emplace_components(a,
			fr::in_place_args<EnemyTag>(),
			fr::in_place_args<PlayerTag>(),
			fr::in_place_args<MyNum>(a_num.value));
		CHECK(res.inserted_count() == 2);

		CHECK(res.was_inserted<EnemyTag>());
		CHECK_FALSE(res.was_inserted<PlayerTag>());
		CHECK(res.was_inserted<MyNum>());

		CHECK(res.get<EnemyTag>() != nullptr);
		CHECK(res.get<PlayerTag>() != nullptr);
		REQUIRE(res.get<MyNum>() != nullptr);
		CHECK(*res.get<MyNum>() == a_num);
	}
}

TEST_CASE("World.moving", "[u][engine][runtime][ecs][world]") {
	const auto a_pos = Pos{1.1f, 0.1f};
	const auto b_pos = Pos{2.2f, 3.3f};
	const auto a_str = MyStr{frt::lorem_text};
	const auto b_str = MyStr{"abcdef"};
	const auto a_data_size = 6zu;
	const auto a_data_val = 25;
	const auto a_data = MyData{a_data_size, a_data_val};

	auto world1 = fr::World<>{};
	auto a = world1.spawn();
	auto b = world1.spawn();

	world1.emplace_components(a,
		fr::in_place_args<Pos>(a_pos.x, a_pos.y),
		a_str,
		fr::in_place_args<MyData>(a_data_size, a_data_val)
	);
	world1.emplace_components(b,
		b_pos,
		b_str
	);

	CHECK(world1.contains(a));
	CHECK(world1.contains(b));

	INFO("move-construct a second world");

	auto world2 = std::move(world1);
	CHECK(world1.is_empty());
	CHECK(world1.entity_count() == 0);
	CHECK(world1.entity_count_with_component<Pos>() == 0);
	CHECK(world1.entity_count_with_component<MyStr>() == 0);
	CHECK(world1.entity_count_with_component<MyData>() == 0);
	CHECK_FALSE(world1.contains(a));
	CHECK_FALSE(world1.contains(b));

	REQUIRE(world2.contains(a));
	REQUIRE(world2.contains(b));
	check_entity_has_component_value(world2, a, a_pos);
	check_entity_has_component_value(world2, a, a_str);
	check_entity_has_component_value(world2, a, a_data);
	check_entity_has_component_value(world2, b, b_pos);
	check_entity_has_component_value(world2, b, b_str);

	INFO("move-assign a third world");

	auto world3 = fr::World<>{};
	world3 = std::move(world2);

	CHECK(world2.is_empty());
	CHECK(world2.entity_count() == 0);
	CHECK(world2.entity_count_with_component<Pos>() == 0);
	CHECK(world2.entity_count_with_component<MyStr>() == 0);
	CHECK(world2.entity_count_with_component<MyData>() == 0);
	CHECK_FALSE(world2.contains(a));
	CHECK_FALSE(world2.contains(b));

	REQUIRE(world3.contains(a));
	REQUIRE(world3.contains(b));
	check_entity_has_component_value(world3, a, a_pos);
	check_entity_has_component_value(world3, a, a_str);
	check_entity_has_component_value(world3, a, a_data);
	check_entity_has_component_value(world3, b, b_pos);
	check_entity_has_component_value(world3, b, b_str);
}

TEST_CASE("World.reserved_memory", "[u][engine][runtime][ecs][world]") {
	SECTION("zero limit") {
		auto world = fr::World<>{{.reserved_memory_limit = 0zu}};
		CHECK(world.reserved_memory() == 0);

		const auto a = world.spawn_with(Pos{2.f, 3.f}, fr::in_place_args<MyStr>("abcdef"));
		CHECK(world.reserved_memory() == 0);

		world.despawn(a);
		CHECK(world.reserved_memory() == 0);
	}
	SECTION("non-zero limit") {
		auto world = fr::World<>{{.reserved_memory_limit = 4096zu}};
		CHECK(world.reserved_memory() == 0);

		const auto a = world.spawn_with(Pos{2.f, 3.f}, fr::in_place_args<MyStr>("abcdef"));
		CHECK(world.reserved_memory() == 0);

		world.despawn(a);
		CHECK(world.reserved_memory() != 0);
	}
}

namespace {

class EntityMask {
public:
	using ValueType = uint64_t;

	using AllComponents = fr::MpList<
		Pos,
		Velocity,
		MyStr,
		MyBuffer,
		MyData,
		MyNum,
		MyChar,
		MySimd
	>;
	static_assert(fr::mp_size<AllComponents> <= CHAR_BIT * sizeof(ValueType));

	static constexpr auto max_size = fr::size_c<fr::mp_size<AllComponents>>;

	EntityMask() = default;

	explicit constexpr
	EntityMask(ValueType bits): _bits{bits} { }

	template<class... Components>
	explicit constexpr
	EntityMask(fr::MpList<Components...>) noexcept {
		(..., set<Components>());
	}

	template<size_t I>
	auto get() const noexcept -> bool {
		static_assert(I < max_size());
		return (_bits & (ValueType{1} << I)) != 0;
	}

	template<class T>
	auto get() const noexcept -> bool {
		static constexpr auto idx = fr::mp_find<AllComponents, T>;
		static_assert(idx != fr::npos);
		return get<idx>();
	}

	template<size_t I>
	void set() noexcept {
		static_assert(I < max_size());
		_bits |= (ValueType{1} << I);
	}

	template<class T>
	void set() noexcept {
		constexpr auto idx = fr::mp_find<AllComponents, T>;
		static_assert(idx != fr::npos);
		set<idx>();
	}

	friend
	auto operator==(EntityMask lhs, EntityMask rhs) -> bool = default;

	[[maybe_unused]] friend
	auto operator^(EntityMask lhs, EntityMask rhs) noexcept -> EntityMask {
		return EntityMask{lhs._bits ^ rhs._bits};
	}

	[[maybe_unused]] friend
	auto operator&(EntityMask lhs, EntityMask rhs) noexcept -> EntityMask {
		return EntityMask{lhs._bits & rhs._bits};
	}

	[[maybe_unused]] friend
	auto operator|(EntityMask lhs, EntityMask rhs) noexcept -> EntityMask {
		return EntityMask{lhs._bits | rhs._bits};
	}

private:
	ValueType _bits = 0;
};

template<fr::c_entity_id EId>
struct EntityState {
	EId eid;
	EntityMask mask;
};

template<fr::c_world TWorld>
class CompTracker {
public:
	using EId = typename TWorld::Entity;

	[[maybe_unused]] friend
	auto operator==(const CompTracker&, const CompTracker&) -> bool = default;

	/// @brief Add random number of components to the entity
	auto add_components_by_idx(TWorld& world, typename TWorld::Entity entity, size_t eidx) {
		// Seems like the easiest way to select N non-repeated elements is `std::shuffle` + take
		// first N elements
		fr::HashDigest32 keys[] = {
			fr::type_hash32<Pos>,
			fr::type_hash32<Velocity>,
			fr::type_hash32<MyStr>,
			fr::type_hash32<MyBuffer>,
			fr::type_hash32<MyData>,
			fr::type_hash32<MyNum>,
			fr::type_hash32<MyChar>,
			fr::type_hash32<MySimd>,
		};
		constexpr auto weights = std::array<int, std::size(keys)>{1, 4, 5, 6, 6, 5, 3, 2};
		static_assert(weights.back() != 0);

		std::ranges::shuffle(keys, _rng);
		auto comp_count = std::discrete_distribution<size_t>{weights.begin(), weights.end()}(_rng);
		auto mask = EntityMask{};
		for (auto i = 0zu; i < comp_count; ++i) {
			switch (keys[i]) {
				case fr::type_hash32<Pos>: {
					const auto v = static_cast<float>(1'000'000 * comp_count + eidx);
					world.add_component(entity, Pos{v, -v});
					mask.set<Pos>();
					break;
				}
				case fr::type_hash32<Velocity>: {
					const auto v = static_cast<float>(1'010'000 * comp_count + eidx);
					world.add_component(entity, Velocity{v});
					mask.set<Velocity>();
					break;
				}
				case fr::type_hash32<MyStr>: {
					auto v = fmt::format("s_{}_{}", comp_count, eidx);
					world.template emplace_component<MyStr>(entity, std::move(v));
					mask.set<MyStr>();
					break;
				}
				case fr::type_hash32<MyBuffer>: {
					world.template emplace_component<MyBuffer>(entity, comp_count, eidx);
					mask.set<MyBuffer>();
					break;
				}
				case fr::type_hash32<MyData>: {
					world.template emplace_component<MyData>(entity, comp_count, eidx);
					mask.set<MyData>();
					break;
				}
				case fr::type_hash32<MyNum>: {
					world.template emplace_component<MyNum>(entity, 1'020'000 * comp_count + eidx);
					mask.set<MyNum>();
					break;
				}
				case fr::type_hash32<MyChar>: {
					world.template emplace_component<MyChar>(entity, 'A' + comp_count);
					mask.set<MyChar>();
					break;
				}
				case fr::type_hash32<MySimd>: {
					world.template emplace_component<MySimd>(entity, comp_count, eidx);
					mask.set<MySimd>();
					break;
				}
				default:
					FR_PANIC_MSG("Missing types");
			}
		}
		_entities.push_back({.eid = entity, .mask = mask});
	}

	void check_all(TWorld& world) const noexcept {
		for (const auto& entity : _entities) {
			// INFO(fmt::format("EID: {}", entity.eid));
			REQUIRE(world.contains(entity.eid));
			fr::unroll<EntityMask::max_size()>([&]<size_t I> {
				if (entity.mask.template get<I>()) {
					using Comp = fr::MpAt<EntityMask::AllComponents, I>;
					// INFO(fmt::format("Component: {}", fr::unqualified_type_name<Comp>));
					CHECK(world.template has_component<Comp>(entity.eid));
					REQUIRE(world.template try_get_component<Comp>(entity.eid) != nullptr);
					CHECK_FALSE(*world.template try_get_component<Comp>(entity.eid) == Comp{});
				}
			});
		}
	}

	template<fr::c_component... RequiredParams, fr::c_component... FilteredOutParams>
	auto filtered_count(
		fr::MpList<RequiredParams...>,
		fr::MpList<FilteredOutParams...>
	) const noexcept -> size_t {
		return static_cast<size_t>(std::ranges::count_if(_entities, [&](const auto& e) {
			const auto req_mask = EntityMask{fr::mp_list<RequiredParams...>};
			const auto filter_mask = EntityMask{fr::mp_list<FilteredOutParams...>};
			return ((e.mask & req_mask) == req_mask)
				&& ((e.mask & filter_mask) == EntityMask{});
		}));
	}

	auto find(EId eid) const noexcept -> std::optional<EntityMask> {
		const auto it = std::ranges::find(_entities, eid, [](const EntityState<EId>& x) {
			return x.eid;
		});
		if (it == _entities.end())
			return std::nullopt;
		return it->mask;
	}

	template<class T>
	auto has(EId eid) const noexcept -> bool {
		const auto state = find(eid);
		if (!state)
			return false;
		return state->template get<T>();
	}

	auto total_count() const noexcept -> size_t { return _entities.size(); }

private:
	std::vector<EntityState<EId>> _entities;
	/// @note Seeded by an arbitrary prime
	std::minstd_rand _rng {1171432692373};
};

} // namespace

TEST_CASE("UncachedQuery::for_each_array", "[u][engine][runtime][ecs][world]") {
	using World = fr::World<>;
	auto world = World{};
	auto tracker = CompTracker<World>{};

	auto entities = std::array<World::Entity, 620>{};
	for (auto i = 0zu; i < std::size(entities); ++i) {
		auto& entity = entities[i];

		entity = world.spawn();
		tracker.add_components_by_idx(world, entity, i);

		REQUIRE(world.entity_count() == tracker.total_count());
		REQUIRE(world.count_entities_in_tables() == tracker.total_count());
	}
	tracker.check_all(world);

	auto received_count = 0zu;

	SECTION("required only, EntityId's #1") {
		auto query = world.build_uncached_query<MyBuffer, MyNum, const Velocity>();
		query.for_each_array([&](
			size_t n,
			const World::Entity*,
			MyBuffer*,
			MyNum*,
			const Velocity*
		) {
			received_count += n;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyBuffer, MyNum, Velocity>,
			fr::mp_list<>
		));
	}
	SECTION("required only, no EntityId's #2") {
		auto query = world.build_uncached_query<MyBuffer, MyNum, const Velocity>();
		query.for_each_array([&](
			size_t n,
			MyBuffer*,
			MyNum*,
			const Velocity*
		) {
			received_count += n;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyBuffer, MyNum, Velocity>,
			fr::mp_list<>
		));
	}
	SECTION("Without<T>, no EntityId's") {
		auto query = world.build_uncached_query<MyBuffer, fr::Without<Velocity>, const MyNum>();
		query.for_each_array([&](
			size_t n,
			MyBuffer*,
			const MyNum*
		) {
			received_count += n;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyBuffer, MyNum>,
			fr::mp_list<Velocity>
		));
	}
	SECTION("With<T>, no EntitId's") {
		auto query = world.build_uncached_query<fr::With<MyChar>, const MyData, const MySimd>();
		query.for_each_array([&](
			size_t n,
			const MyData*,
			const MySimd*
		) {
			received_count += n;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyChar, MyData, MySimd>,
			fr::mp_list<>
		));
	}
	SECTION("Has<T>, EntitId's") {
		auto query = world.build_uncached_query<fr::With<MyChar>, const MyData, const MySimd,
			fr::Has<Pos>>();
		query.for_each_array([&](
			size_t n,
			const World::Entity* eids,
			const MyData*,
			const MySimd*,
			fr::Has<Pos> has_pos
		) {
			received_count += n;
			for (size_t i = 0; i < n; ++i) {
				CHECK(tracker.has<Pos>(eids[i]) == has_pos.value());
			}
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyChar, MyData, MySimd>,
			fr::mp_list<>
		));
	}
	SECTION("optional, EntityId's") {
		auto query = world.build_uncached_query<MyNum*, const MySimd, const MyChar*, const Velocity,
			fr::Without<MyBuffer>>();
		query.for_each_array([&](
			size_t n,
			const World::Entity* eids,
			MyNum* num,
			const MySimd*,
			const MyChar* ch,
			const Velocity*
		) {
			received_count += n;
			for (size_t i = 0; i < n; ++i) {
				CHECK(tracker.has<MyNum>(eids[i]) == (num != nullptr));
				CHECK(tracker.has<MyChar>(eids[i]) == (ch != nullptr));
			}
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MySimd, Velocity>,
			fr::mp_list<MyBuffer>
		));
	}
	SECTION("everything together, EntityId's") {
		auto query = world.build_uncached_query<
			const MySimd,
			const MyChar*,
			fr::With<MyStr>,
			fr::Without<MyNum>,
			Pos,
			fr::Has<Velocity>,
			fr::Without<MyBuffer>
		>();
		query.for_each_array([&](
			size_t n,
			const World::Entity* eids,
			const MySimd* simd,
			const MyChar* ch,
			Pos* pos,
			fr::Has<Velocity> has_vel
		) {
			received_count += n;
			CHECK(simd != nullptr);
			CHECK(pos != nullptr);
			for (size_t i = 0; i < n; ++i) {
				CHECK(tracker.has<MyChar>(eids[i]) == (ch != nullptr));
				CHECK(tracker.has<Velocity>(eids[i]) == has_vel.value());
			}
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MySimd, Pos, MyStr>,
			fr::mp_list<MyNum, MyBuffer>
		));
	}
}

TEST_CASE("UncachedQuery::for_each", "[u][engine][runtime][ecs][world]") {
	using World = fr::World<>;
	auto world = World{};
	auto tracker = CompTracker<World>{};

	auto entities = std::array<World::Entity, 620>{};
	for (auto i = 0zu; i < std::size(entities); ++i) {
		auto& entity = entities[i];

		entity = world.spawn();
		tracker.add_components_by_idx(world, entity, i);

		REQUIRE(world.entity_count() == tracker.total_count());
		REQUIRE(world.count_entities_in_tables() == tracker.total_count());
	}

	auto received_count = 0zu;
	SECTION("required only, EntityId's #1") {
		auto query = world.build_uncached_query<MyBuffer, MyNum, const Velocity>();
		query.for_each([&](
			World::Entity,
			MyBuffer&,
			MyNum&,
			const Velocity&
		) {
			++received_count;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyBuffer, MyNum, Velocity>,
			fr::mp_list<>
		));
	}
	SECTION("required only, no EntityId's #2") {
		auto query = world.build_uncached_query<MyBuffer, MyNum, const Velocity>();
		query.for_each([&](
			MyBuffer&,
			MyNum&,
			const Velocity&
		) {
			++received_count;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyBuffer, MyNum, Velocity>,
			fr::mp_list<>
		));
	}
	SECTION("Without<T>, no EntityId's") {
		auto query = world.build_uncached_query<MyBuffer, fr::Without<Velocity>, const MyNum>();
		query.for_each([&](
			MyBuffer&,
			const MyNum&
		) {
			++received_count;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyBuffer, MyNum>,
			fr::mp_list<Velocity>
		));
	}
	SECTION("With<T>, no EntitId's") {
		auto query = world.build_uncached_query<fr::With<MyChar>, const MyData, const MySimd>();
		query.for_each([&](
			const MyData&,
			const MySimd&
		) {
			++received_count;
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyChar, MyData, MySimd>,
			fr::mp_list<>
		));
	}
	SECTION("Has<T>, EntitId's") {
		auto query = world.build_uncached_query<fr::With<MyChar>, const MyData, const MySimd,
			fr::Has<Pos>>();
		query.for_each([&](
			World::Entity eid,
			const MyData&,
			const MySimd&,
			fr::Has<Pos> has_pos
		) {
			++received_count;
			CHECK(tracker.has<Pos>(eid) == has_pos.value());
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MyChar, MyData, MySimd>,
			fr::mp_list<>
		));
	}
	SECTION("optional, EntityId's") {
		auto query = world.build_uncached_query<MyNum*, const MySimd, const MyChar*, const Velocity,
			fr::Without<MyBuffer>>();
		query.for_each([&](
			const World::Entity& eids,
			MyNum* num,
			const MySimd&,
			const MyChar* ch,
			const Velocity&
		) {
			++received_count;
			CHECK(tracker.has<MyNum>(eids) == (num != nullptr));
			CHECK(tracker.has<MyChar>(eids) == (ch != nullptr));
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MySimd, Velocity>,
			fr::mp_list<MyBuffer>
		));
	}
	SECTION("everything together, EntityId's") {
		auto query = world.build_uncached_query<
			const MySimd,
			const MyChar*,
			fr::With<MyStr>,
			fr::Without<MyNum>,
			Pos,
			fr::Has<Velocity>,
			fr::Without<MyBuffer>
		>();
		query.for_each([&](
			World::Entity eid,
			const MySimd&,
			const MyChar* ch,
			Pos&,
			fr::Has<Velocity> has_vel
		) {
			++received_count;
			CHECK(tracker.has<MyChar>(eid) == (ch != nullptr));
			CHECK(tracker.has<Velocity>(eid) == has_vel.value());
		});
		CHECK(received_count == tracker.filtered_count(
			fr::mp_list<MySimd, Pos, MyStr>,
			fr::mp_list<MyNum, MyBuffer>
		));
	}
}
