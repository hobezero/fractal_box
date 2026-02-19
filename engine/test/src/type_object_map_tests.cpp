#include "fractal_box/core/containers/type_object_map.hpp"

#include <set>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/default_utils.hpp"

#include "test_common/test_helpers.hpp"
#include "test_common/type_index_fmt.hpp"

namespace {

struct TestDomain: fr::CustomTypeIndexDomainBase<> { };
using TestTypeIdx = fr::TypeIndex<TestDomain>;

struct IterationDomain: fr::CustomTypeIndexDomainBase<> { };
using IterationTypeIdx = fr::TypeIndex<IterationDomain>;

} // namespace

TEST_CASE("TypeObjectMap.Iterators", "[u][engine][core][meta]") {
	[[maybe_unused]] static auto string_idx = IterationTypeIdx::of<std::string>;
	[[maybe_unused]] static auto char_idx = IterationTypeIdx::of<char>;
	[[maybe_unused]] static auto unsigned_idx = IterationTypeIdx::of<unsigned>;
	[[maybe_unused]] static auto long_idx = IterationTypeIdx::of<long>;
	[[maybe_unused]] static auto short_idx = IterationTypeIdx::of<short>;

	SECTION("traits") {
		using Map = fr::TypeObjectMap<TestDomain, char>;
		const Map cm;
		Map m;
		CHECK(std::ranges::forward_range<Map>);
		CHECK(std::forward_iterator<decltype(m.cbegin())>);
		CHECK(std::forward_iterator<decltype(cm.begin())>);
		CHECK(std::forward_iterator<decltype(m.begin())>);
	}
	SECTION("for loop") {
		using Item = std::pair<IterationTypeIdx, int>;
		const Item inserted[] = {
			{char_idx, 10},
			{unsigned_idx, 20},
			{short_idx, 30},
		};

		auto map = fr::TypeObjectMap<IterationDomain, int>{};
		for (auto [idx, num] : inserted)
			map.try_emplace_at(idx, num);

		auto visited = std::multiset<Item>{};
		for (auto it = map.begin(); it != map.end(); ++it)
			visited.emplace(it->first, it->second);

		CHECK(map.size() == std::size(inserted));
		CHECK(visited == std::multiset<Item>(std::begin(inserted), std::end(inserted)));
	}
	SECTION("Non-const to const iterator conversion") {
		auto map = fr::TypeObjectMap<IterationDomain, int>{};
		map.try_emplace_at(char_idx, 12);
		CHECK((map.begin() == map.cbegin()));
	}
}

TEST_CASE("TypeAnyObjectMap", "[u][engine][core][meta]") {
	[[maybe_unused]] static auto string_idx = TestTypeIdx::of<std::string>;
	[[maybe_unused]] static auto char_idx = TestTypeIdx::of<char>;
	[[maybe_unused]] static auto unsigned_idx = TestTypeIdx::of<unsigned>;

	auto map = fr::TypeAnyObjectMap<TestDomain>{};

	CHECK(map.try_get<unsigned>() == nullptr);
	CHECK(std::as_const(map).try_get<unsigned>() == nullptr);

	CHECK(frt::points_to_value(map.try_emplace<unsigned>(12u).where, 12u));
	REQUIRE(map.try_get<unsigned>() != nullptr);
	CHECK(*map.try_get<unsigned>() == 12u);
	CHECK(*std::as_const(map).try_get<unsigned>() == 12u);

	CHECK(frt::points_to_value(map.try_emplace<char>('q').where, 'q'));

	const auto str = std::string("1234567901234567890123456789012345678901234567890");
	CHECK(frt::points_to_value(map.try_emplace<std::string>(str).where, str));
	REQUIRE(map.try_get<unsigned>() != nullptr);
	CHECK(*map.try_get<unsigned>() == 12u);
	CHECK(*std::as_const(map).try_get<unsigned>() == 12u);

	REQUIRE(map.try_get<std::string>() != nullptr);
	CHECK(*map.try_get<std::string>() == str);
	CHECK(*std::as_const(map).try_get<std::string>() == str);

	CHECK_FALSE(map.try_emplace<unsigned>(15u));
	CHECK_FALSE(map.try_emplace<std::string>());
}

namespace {

struct TestOrderDomain: fr::CustomTypeIndexDomainBase<> {
	static constexpr auto init_value = ValueType{1};
	static constexpr auto null_value = ValueType{0};
};

using TestOrderTypeIdx = fr::TypeIndex<TestOrderDomain>;

static std::vector<TestOrderTypeIdx::ValueType> construction_order;
static std::vector<TestOrderTypeIdx::ValueType> destruction_order;

template<class T>
struct DestructionOrderSpy {
	explicit
	DestructionOrderSpy():
		_idx{TestOrderTypeIdx::of<T>.value()}
	{
		construction_order.push_back(_idx.value());
	}

	DestructionOrderSpy(const DestructionOrderSpy&) = delete;
	auto operator=(const DestructionOrderSpy&) -> DestructionOrderSpy& = delete;

	DestructionOrderSpy(DestructionOrderSpy&&) noexcept = default;
	auto operator=(DestructionOrderSpy&&) noexcept -> DestructionOrderSpy& = default;

	~DestructionOrderSpy() {
		if (!_idx.is_default()) {
			destruction_order.push_back(_idx.value());
		}
	}

private:
	fr::WithDefaultValue<TestOrderDomain::null_value> _idx;
};

} // namespace

TEST_CASE("TypeAnyObjectMap.DestructionOrder", "[u][engine][core][meta]") {
	[[maybe_unused]] static auto string_idx = TestOrderTypeIdx::of<std::string>; // 1
	[[maybe_unused]] static auto char_idx = TestOrderTypeIdx::of<char>; // 2
	[[maybe_unused]] static auto unsigned_idx = TestOrderTypeIdx::of<unsigned>; // 3
	[[maybe_unused]] static auto long_idx = TestOrderTypeIdx::of<long>; // 4
	[[maybe_unused]] static auto short_idx = TestOrderTypeIdx::of<short>; // 5

	{
		auto map = fr::TypeAnyObjectMap<TestOrderDomain>{};

		map.try_emplace<DestructionOrderSpy<long>>();
		map.try_emplace<DestructionOrderSpy<unsigned>>();
		map.try_emplace<DestructionOrderSpy<std::string>>();
		map.try_emplace<DestructionOrderSpy<short>>();

		map.try_emplace<DestructionOrderSpy<std::string>>();
		map.try_emplace<DestructionOrderSpy<long>>();

		// Moves must not affect anything
		auto map2 = std::move(map);
	}

	auto expected_order = construction_order;
	std::ranges::reverse(expected_order);
	CHECK(destruction_order == expected_order);
}
