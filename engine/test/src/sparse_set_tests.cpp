#include "fractal_box/core/containers/sparse_set.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

template<class Traits>
static constexpr typename Traits::RawType locations[] = {
	0, 1, 2, 124,
	Traits::max_location / 2,
	Traits::max_location - 1,
	Traits::max_location,
};

template<class Traits>
static constexpr typename Traits::RawType versions[] = {
	0, 1, 2, 124,
	Traits::max_version / 2,
	Traits::max_version - 1,
	Traits::max_version,
};

TEST_CASE("BitmaskSparseKeyTraits", "[u][engine][core][containers]") {
	using Key = uint32_t;
	using Traits = fr::BitmaskSparseKeyTraits<Key>;

	SECTION("make(..)") {
		for (const auto loc : locations<Traits>) {
			for (const auto ver : versions<Traits>) {
				INFO(fmt::format("location: {}, version: {}", loc, ver));
				const auto key = Traits::make(loc, ver);
				REQUIRE(Traits::is_valid(key));
				CHECK(Traits::location(key) == loc);
				CHECK(Traits::version(key) == ver);
			}
		}
	}
	SECTION("make_invalidated(..)") {
		for (const auto loc : locations<Traits>) {
			for (const auto ver : versions<Traits>) {
				INFO(fmt::format("location: {}, version: {}", loc, ver));
				const auto valid_key = Traits::make(loc, ver);
				const auto invalid_key = Traits::make_invalidated(valid_key);
				CHECK_FALSE(Traits::is_valid(invalid_key));
				CHECK(Traits::version(invalid_key) == Traits::version(valid_key));
			}
		}
	}
	SECTION("make_next(..)") {
		for (const auto loc : locations<Traits>) {
			for (const auto ver : versions<Traits>) {
				INFO(fmt::format("location: {}, version: {}", loc, ver));
				const auto first_key = Traits::make(loc, ver);
				const auto second_key = Traits::make_next(first_key);
				if (ver == Traits::max_version) {
					CHECK(Traits::version(second_key) == 0);
				}
				else {
					CHECK(static_cast<int64_t>(Traits::version(second_key))
						- static_cast<int64_t>(Traits::version(first_key)) == 1);
				}
			}
		}
	}
	SECTION("null") {
		CHECK_FALSE(Traits::is_valid(Traits::null));
	}
}

template<class Set>
static
void check_contains(Set& set, auto key) {
	CHECK_FALSE(set.empty());
	const bool contains = set.contains(key);
	CHECK(contains);
	const auto loc1 = set.try_get_location(key);
	CHECK(loc1 != fr::npos_for<typename Set::RawType>);
	if (!contains)
		return;
	const auto loc2 = set.get_location_unchecked(key);
	if (loc1 != fr::npos_for<typename Set::RawType>)
		CHECK(loc1 == loc2);
}

template<class Set>
static
void check_doesnt_contain(Set& set, auto key) {
	CHECK(!set.contains(key));
	CHECK(set.try_get_location(key) == fr::npos_for<typename Set::RawType>);
}

template<class Map>
static
void check_contains_value(Map& map, auto key, const auto& value) {
	CHECK_FALSE(map.empty());
	const bool contains = map.contains(key);
	CHECK(contains);

	const auto loc1 = map.try_get_location(key);
	CHECK(loc1 != fr::npos_for<typename Map::RawType>);
	if (!contains)
		return;
	const auto loc2 = map.get_location_unchecked(key);
	if (loc1 != fr::npos_for<typename Map::RawType>)
		CHECK(loc1 == loc2);
	auto* const ptr = map.try_get_value(key);

	REQUIRE(ptr);
	CHECK(*ptr == value);
	CHECK(map.get_value_unchecked(key) == value);
}

namespace {

enum class MySparseKey: uint32_t { };

struct MyKeyValuePair {
	MySparseKey key;
	std::string value;
};

} // namespace

TEST_CASE("SparseIndexSet", "[u][engine][core][containers]") {
	using Set = fr::SparseIndexSet<MySparseKey>;
	using Key = typename Set::KeyType;
	using Traits = typename Set::TraitsType;
	auto set = Set{};

	SECTION("type properties") {
		CHECK(std::is_default_constructible_v<Set>);
		CHECK(std::is_nothrow_default_constructible_v<Set>);
		CHECK(std::is_copy_constructible_v<Set>);
		CHECK(std::is_copy_assignable_v<Set>);
		CHECK(std::is_move_constructible_v<Set>);
		CHECK(std::is_move_assignable_v<Set>);
		CHECK(std::is_nothrow_move_constructible_v<Set>);
		CHECK(std::is_nothrow_move_assignable_v<Set>);
	}
	SECTION("initialized set is empty") {
		CHECK(set.empty());
		CHECK(set.size() == 0); // NOLINT

		SECTION("empty set contains no elements") {
			for (const auto loc : locations<Traits>) {
				for (const auto ver : versions<Traits>) {
					CHECK(!set.contains(Traits::make(loc, ver)));
				}
			}
		}
		SECTION("range is empty") {
			CHECK(set.begin() == set.end());
			CHECK(set.cbegin() == set.cend());
			CHECK(std::as_const(set).begin() == std::as_const(set).end());
			CHECK(std::as_const(set).cbegin() == std::as_const(set).cend());
		}
	}
	SECTION("add one element") {
		const auto key = set.create();
		check_contains(set, key);
		CHECK(*set.begin() == key);
		CHECK(set.size() == 1);
		CHECK(set.begin() + 1 == set.end());
	}
	SECTION("add multiple elements") {
		std::vector<Key> current_keys(20);
		for (auto& key : current_keys)
			key = set.create();

		CHECK(set.size() == current_keys.size());
		for (const auto key : current_keys)
			check_contains(set, key);

		INFO("erase some of the elements");
		std::vector<Key> erased_keys;
		// TODO: erase in reverse order
		for (size_t i = 0; i < current_keys.size(); i += 3) {
			const auto key = current_keys[i];
			set.erase(key);
			erased_keys.push_back(key);
		}
		for (const auto key : erased_keys)
			std::erase(current_keys, key);

		CHECK(set.size() == current_keys.size());
		for (const auto key : current_keys)
			check_contains(set, key);
		for (const auto key : erased_keys)
			check_doesnt_contain(set, key);

		INFO("add more elements");
		std::vector<Key> new_keys(erased_keys.size() / 2);
		for (auto& key : new_keys) {
			key = set.create();
			current_keys.push_back(key);
		}
		for (const auto key : current_keys)
			check_contains(set, key);

		INFO("fill the container to the previous size");
		for (size_t i = 0; i < erased_keys.size() / 2; ++i) {
			const auto key = set.create();
			new_keys.push_back(key);
			current_keys.push_back(key);
		}
		CHECK(set.size() == current_keys.size());
		for (const auto key : current_keys)
			check_contains(set, key);

		INFO("overfill the container");
		for (size_t i = 0; i < erased_keys.size() / 2; ++i) {
			const auto key = set.create();
			new_keys.push_back(key);
			current_keys.push_back(key);
		}
		CHECK(set.size() == current_keys.size());
		for (const auto key : current_keys)
			check_contains(set, key);

		erased_keys.clear();
		INFO("erase a few of the last keys");
		for (size_t i = 0; i < 2; ++i) {
			const auto key = set.keys().back();
			set.erase(key);
			erased_keys.push_back(key);
			std::erase(current_keys, key);
		}

		INFO("erase some of the keys (again)");
		for (size_t i = 0; i < current_keys.size(); i += 2) {
			const auto key = current_keys[i];
			set.erase(key);
			erased_keys.push_back(key);
		}
		for (const auto key : erased_keys)
			std::erase(current_keys, key);

		CHECK(set.size() == current_keys.size());
		for (const auto key : current_keys)
			check_contains(set, key);
		for (const auto key : erased_keys)
			check_doesnt_contain(set, key);

		INFO("clear the set");
		set.clear();
		CHECK(set.empty());
		CHECK(set.begin() == set.end());
		CHECK(set.size() == 0); // NOLINT
	}
}

TEST_CASE("SparseMap", "[u][engine][core][containers]") {
	using Map = fr::SparseMap<MySparseKey, std::string>;
	using Traits = typename Map::TraitsType;

	auto set = Map{};
	auto counter = 0;

	const auto make_value = [&] { return fmt::format("{}", counter++); };
	const auto make_value_tagged = [&](auto&& tag) { return fmt::format("{}-{}", counter++, tag); };

	SECTION("type properties") {
		CHECK(std::is_default_constructible_v<Map>);
		CHECK(std::is_nothrow_default_constructible_v<Map>);
		CHECK(std::is_copy_constructible_v<Map>);
		CHECK(std::is_copy_assignable_v<Map>);
		CHECK(std::is_move_constructible_v<Map>);
		CHECK(std::is_move_assignable_v<Map>);
		CHECK(std::is_nothrow_move_constructible_v<Map>);
		CHECK(std::is_nothrow_move_assignable_v<Map>);
	}
	SECTION("initialized set is empty") {
		CHECK(set.empty());
		CHECK(set.size() == 0); // NOLINT

		SECTION("empty set contains no elements") {
			for (const auto loc : locations<Traits>) {
				for (const auto ver : versions<Traits>) {
					CHECK(!set.contains(Traits::make(loc, ver)));
				}
			}
		}
#if 0
		SECTION("range is empty") {
			CHECK(set.begin() == set.end());
			CHECK(set.cbegin() == set.cend());
			CHECK(std::as_const(set).begin() == std::as_const(set).end());
			CHECK(std::as_const(set).cbegin() == std::as_const(set).cend());
		}
#endif
	}
	SECTION("add one element") {
		const auto key = set.insert("abc");
		check_contains_value(set, key, std::string("abc"));
		CHECK(set.size() == 1);
#if 0
		CHECK(*set.begin() == key);
		CHECK(set.begin() + 1 == set.end());
#endif
	}
	SECTION("add multiple elements") {
		auto current_keys = std::vector<MyKeyValuePair>(20);
		for (auto& [key, value] : current_keys) {
			value = make_value();
			key = set.insert(value);
		}

		CHECK(set.size() == current_keys.size());
		for (const auto& [key, value] : current_keys)
			check_contains_value(set, key, value);

		INFO("erase some of the elements");
		auto erased_keys = std::vector<MyKeyValuePair>();
		// TODO: erase in reverse order
		for (auto i = 0uz; i < current_keys.size(); i += 3uz) {
			const auto pair = current_keys[i];
			set.erase(pair.key);
			erased_keys.push_back(pair);
		}
		for (const auto& [key, value] : erased_keys)
			std::erase_if(current_keys, [&] (auto&& pair) { return pair.key == key; });

		CHECK(set.size() == current_keys.size());
		for (const auto& [key, value] : current_keys)
			check_contains_value(set, key, value);
		for (const auto& [key, value] : erased_keys)
			check_doesnt_contain(set, key);

#if 1
		INFO("add more elements");
		auto new_keys = std::vector<MyKeyValuePair>(erased_keys.size() / 2uz);
		for (auto& [key, value] : new_keys) {
			value = make_value_tagged("MORE");
			key = set.insert(value);
			current_keys.push_back({key, value});
		}
		for (const auto& [key, value] : current_keys)
			check_contains_value(set, key, value);

		INFO("fill container to the previous size");
		for (auto i = 0uz; i < erased_keys.size() / 2uz; ++i) {
			// erased_keys[i].value =
			auto value = make_value_tagged("FILL");
			const auto key = set.insert(value);
			new_keys.push_back({key, value});
			current_keys.push_back({key, value});
		}
		CHECK(set.size() == current_keys.size());
		for (const auto& [key, value] : current_keys)
			check_contains_value(set, key, value);

		INFO("overfill container");
		for (auto i = 0uz; i < erased_keys.size() / 2uz; ++i) {
			auto value = make_value_tagged("OFILL");
			const auto key = set.insert(value);
			new_keys.push_back({key, value});
			current_keys.push_back({key, value});
		}
		CHECK(set.size() == current_keys.size());
		for (const auto& [key, value] : current_keys)
			check_contains_value(set, key, value);

		INFO("erase some of the keys (again)");
		erased_keys.clear();
		for (auto i = 0uz; i < current_keys.size(); i += 2uz) {
			const auto& pair = current_keys[i];
			set.erase(pair.key);
			erased_keys.push_back(pair);
		}
		for (const auto& [key, value] : erased_keys)
			std::erase_if(current_keys, [&] (auto&& pair) { return pair.key == key; });

		CHECK(set.size() == current_keys.size());
		for (const auto& [key, value] : current_keys)
			check_contains_value(set, key, value);
		for (const auto& [key, value] : erased_keys)
			check_doesnt_contain(set, key);

		INFO("clear the set");
		set.clear();
		CHECK(set.empty());
#if 0
		CHECK(set.begin() == set.end());
#endif
		CHECK(set.size() == 0); // NOLINT
#endif
	}
}
