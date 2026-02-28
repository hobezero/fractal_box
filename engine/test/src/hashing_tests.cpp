#include "fractal_box/core/hashing/rapidhash.hpp"
#include "fractal_box/core/hashing/uni_hasher.hpp"

#include <any>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "fractal_box/core/meta/meta.hpp"
#include "test_common/test_helpers.hpp"
#include "test_common/hashing_fmt.hpp"

using namespace fr::hash_literals;
using namespace std::string_view_literals;
using namespace std::string_literals;
using enum fr::UniHasherSeeding;
using HVB = fr::HasherVisitorBase;

// Rapidhash tests
// ---------------

namespace {

struct HashTuple {
	auto data() const noexcept -> const unsigned char* {
		return reinterpret_cast<const unsigned char*>(str.data());
	}

	auto size() const noexcept { return str.size(); }

public:
	std::string_view str;
	fr::HashDigest64 digest_default;
	fr::HashDigest64 digest_micro;
	fr::HashDigest64 digest_nano;
};

} // namespace

TEST_CASE("rapidhash.values", "[u][engine][core][hashing]") {
	static constexpr uint64_t seed = UINT64_C(0x2D4523AF012EB42F);
	static constexpr HashTuple inputs[] = {
		{
			"",
			UINT64_C(0xC745AE2AB0BE7C88),
			UINT64_C(0xC745AE2AB0BE7C88),
			UINT64_C(0xC745AE2AB0BE7C88)
		},
		{
			"a",
			UINT64_C(0x9CCE84C377E8A7F0),
			UINT64_C(0x9CCE84C377E8A7F0),
			UINT64_C(0x9CCE84C377E8A7F0)
		},
		{
			"123",
			UINT64_C(0xF5F1603E86451DD6),
			UINT64_C(0xF5F1603E86451DD6),
			UINT64_C(0xF5F1603E86451DD6)
		},
		{
			"abcd",
			UINT64_C(0x9E3D2E040E79FEAB),
			UINT64_C(0x9E3D2E040E79FEAB),
			UINT64_C(0x9E3D2E040E79FEAB)
		},
		{
			"ABCDE",
			UINT64_C(0x11A256F2B39E10D1),
			UINT64_C(0x11A256F2B39E10D1),
			UINT64_C(0x11A256F2B39E10D1)
		},
		{
			"ABCDE ASJFLKSDJFLKJ",
			UINT64_C(0x70636f872b71243e),
			UINT64_C(0x70636f872b71243e),
			UINT64_C(0x70636f872b71243e)
		},
		{
			"The quick brown fox jumps over the lazy dog: 1234567890123456790",
			UINT64_C(0xC5C7F5AA1229793F),
			UINT64_C(0xC5C7F5AA1229793F),
			UINT64_C(0x1671FAB6006088FF)
		},
		{
			frt::lorem_text,
			UINT64_C(0xE6B1F7B76C7A7040),
			UINT64_C(0xC29892C2E9ACA3B5),
			UINT64_C(0xD0898B2FC165DC7F)
		},
		{
			frt::lorem_text_long,
			UINT64_C(0xED772F9133FCE972),
			UINT64_C(0x3785CF8262AB78BE),
			UINT64_C(0x3B0D01C6C87D7F4F)
		},
	};

	SECTION("rapidhash") {
		using RH = fr::Rapidhash<true, false, true>;
		for (const auto& x : inputs) {
			INFO(fmt::format("({}): '{}'", x.size(), x.str));
			CHECK(RH::hash_bytes_seeded(x.data(), x.size(), seed) == x.digest_default);
		}
	}
	SECTION("rapidhash_micro") {
		using RH = fr::RapidhashMicro<true, false>;
		for (const auto& x : inputs) {
			INFO(fmt::format("({}): '{}'", x.size(), x.str));
			CHECK(RH::hash_bytes_seeded(x.data(), x.size(), seed) == x.digest_micro);
		}
	}
	SECTION("rapidhash_nano") {
		using RH = fr::RapidhashNano<true, false>;
		for (const auto& x : inputs) {
			INFO(fmt::format("({}): '{}'", x.size(), x.str));
			CHECK(RH::hash_bytes_seeded(x.data(), x.size(), seed) == x.digest_nano);
		}
	}
}

TEST_CASE("hash_mix_commutative") {
	STATIC_CHECK(fr::hash_mix_commutative(0x1020_digest16, 0x1020_digest16)
		!= fr::HashDigest16{});
	STATIC_CHECK(fr::hash_mix_commutative(0x1020_digest32, 0x1929_digest32)
		== fr::hash_mix_commutative(0x1929_digest32, 0x1020_digest32));

	STATIC_CHECK(fr::hash_mix_commutative(0x10203040_digest32, 0x10203040_digest32)
		!= fr::HashDigest32{});
	STATIC_CHECK(fr::hash_mix_commutative(0x10203040_digest32, 0x19293949_digest32)
		== fr::hash_mix_commutative(0x19293949_digest32, 0x10203040_digest32));

	STATIC_CHECK(fr::hash_mix_commutative(0x1020304050607080_digest64, 0x1020304050607080_digest64)
		!= fr::HashDigest32{});
	STATIC_CHECK(fr::hash_mix_commutative(0x1020304050607080_digest64, 0x1929394959697989_digest64)
		== fr::hash_mix_commutative(0x1929394959697989_digest64, 0x1020304050607080_digest64));
}

// Hashability tests
// -----------------

namespace {

enum class SimpleEnum: unsigned {
	E0,
	E1 = 23,
	E2,
	E3,
	E4,
	E5,
};

struct CustomStruct {
public:
	template<class H>
	friend constexpr
	void kepler_custom_hash(const CustomStruct& self, H& visitor) {
		visitor(self.x, self.y, self.z, self.w);
	}

public:
	int x;
	int y;
	float z;
	bool w;
};

class AbstractClass {
public:
	virtual ~AbstractClass() = 0;
};

class PrivateClass {
private:
	[[maybe_unused]]
	int _foo;
};

struct DescribedNonHashableClass {
	using Self = DescribedNonHashableClass;

	constexpr
	DescribedNonHashableClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z>
		>;
	}

private:
	int _x;
	int _y;
	std::string _z;
};

struct DescribedOptOutClass {
	using Self = DescribedOptOutClass;

	constexpr
	DescribedOptOutClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::HashableMode::OptOut>,
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z, fr::Attributes<fr::Hashable{false}>>
		>;
	}

private:
	int _x;
	int _y;
	std::string _z;
};

struct DescribedOptInClass {
	using Self = DescribedOptInClass;

	constexpr
	DescribedOptInClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::HashableMode::OptIn>,
			fr::Field<&Self::_x, fr::Attributes<fr::Hashable{}>>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z, fr::Attributes<fr::Hashable{}>>
		>;
	}

private:
	int _x;
	int _y;
	std::string _z;
};

struct DescribedAsBytesClass {
	using Self = DescribedAsBytesClass;

	constexpr
	DescribedAsBytesClass(int x, int y, int z): _x{x}, _y{y}, _z{z} { }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::HashableMode::AsBytes>,
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z>
		>;
	}

private:
	int _x;
	int _y;
	int _z;
};

struct DescribedNoneClass {
	using Self = DescribedNoneClass;

	constexpr
	DescribedNoneClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::HashableMode::None>,
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z>
		>;
	}

private:
	int _x;
	int _y;
	std::string _z;
};

struct DescribedHashableClass {
	using Self = DescribedHashableClass;

	constexpr
	DescribedHashableClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::Hashable{}>,
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z>
		>;
	}

private:
	int _x;
	int _y;
	std::string _z;
};

struct DescribedHashableFalseClass {
	using Self = DescribedHashableFalseClass;

	constexpr
	DescribedHashableFalseClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::Hashable{false}>,
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z>
		>;
	}

private:
	int _x;
	int _y;
	std::string _z;
};

struct BaseA {
	[[maybe_unused]] constexpr
	auto operator==(const BaseA&) const -> bool = default;

public:
	SimpleEnum a;
};

struct BaseB {
	[[maybe_unused]] friend consteval
	auto kepler_describe(const BaseB&) {
		return fr::class_desc<
			fr::Attributes<fr::Hashable{}>,
			fr::Field<&BaseB::b>,
			fr::Field<&BaseB::c>
		>;
	}

public:
	int16_t b;
	std::string c;
};

struct BaseC {
	float* ptr = nullptr;
};

struct DescribedHashableWithBasesAndProps: public BaseA, public BaseB, public BaseC {
	using Self = DescribedHashableWithBasesAndProps;

	constexpr
	DescribedHashableWithBasesAndProps(
		SimpleEnum a_val,
		int16_t b_val,
		std::string c_val,
		int x,
		std::string y,
		int64_t z
	):
		BaseA{a_val},
		BaseB{b_val, std::move(c_val)},
		_x{x},
		_y{std::move(y)},
		_z{z}
	{ }

	[[maybe_unused]] friend consteval
	auto kepler_describe(const Self&) {
		return fr::class_desc<
			fr::Bases<BaseA, BaseB, BaseC>,
			fr::Attributes<fr::HashableMode::OptIn>,
			fr::Field<&Self::_x>,
			fr::Property<"y", std::string_view, &Self::y, nullptr, fr::Attributes<fr::Hashable{}>>,
			fr::Field<&Self::_z, fr::Attributes<fr::Hashable{}>>
		>;
	}

	auto y() const noexcept -> std::string_view { return _y; }

private:
	int _x;
	std::string _y;
	int64_t _z;
};

struct HashableAggregate {
	int32_t i;
	std::string s;
	std::array<int64_t, 2> a;
	std::tuple<std::string, int8_t, std::string> a2;
};

struct ByteHashableAggregate {
	uint32_t a;
	int16_t b;
	int16_t c;
};

struct PaddedHashableAggregate {
	uint32_t a;
	int16_t b;
};

struct UnhashableAggregate {
	void* p1;
	int* p2;
	std::string s;
};

static constexpr
auto simple_view() {
	return std::views::iota(1) | std::views::take(5);
}

static constexpr
auto unhashable_view() {
	return std::views::single(UnhashableAggregate{});
}

} // namespace

TEST_CASE("get_hashability", "[u][engine][core][hashing]") {
	using enum fr::HashableCategory;
	using enum fr::HashableMode;
	using HA = fr::Hashability;

	SECTION("Primitive") {
		STATIC_CHECK(fr::get_hashability<bool>() == HA{Primitive, AsBytes});
		STATIC_CHECK(fr::get_hashability<int>() == HA{Primitive, AsBytes});
		STATIC_CHECK(fr::get_hashability<unsigned long>() == HA{Primitive, AsBytes});
		STATIC_CHECK(fr::get_hashability<float>() == HA{Primitive, OptOut});
		STATIC_CHECK(fr::get_hashability<long double>() == HA{Primitive, OptOut});
		STATIC_CHECK(fr::get_hashability<std::nullptr_t>() == HA{Primitive, AsBytes});
		STATIC_CHECK(fr::get_hashability<void>() == HA{Primitive, None});
	}
	SECTION("Wrapper") {
		STATIC_CHECK(fr::get_hashability<HVB::Digest<fr::HashDigest32>>() == HA{Wrapper, AsBytes});

		STATIC_CHECK(fr::get_hashability<HVB::Bytes<char, size_t>>() == HA{Wrapper, OptOut});

		STATIC_CHECK(fr::get_hashability<HVB::FixedBytes<char, 6>>() == HA{Wrapper, OptOut});

		STATIC_CHECK(fr::get_hashability<HVB::String<char16_t, size_t>>() == HA{Wrapper, OptOut});

		STATIC_CHECK(fr::get_hashability<HVB::FixedString<char16_t, 6>>() == HA{Wrapper, OptOut});

		STATIC_CHECK(fr::get_hashability<HVB::Ptr<int>>() == HA{Wrapper, AsBytes});
		STATIC_CHECK(fr::get_hashability<HVB::Ptr<PrivateClass>>() == HA{Wrapper, AsBytes});
		STATIC_CHECK(fr::get_hashability<HVB::Ptr<UnhashableAggregate>>() == HA{Wrapper, AsBytes});

		STATIC_CHECK(fr::get_hashability<HVB::Tuple<>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::Tuple<int>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::Tuple<int, float>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::Tuple<int, UnhashableAggregate>>()
			== HA{Wrapper, None});

		STATIC_CHECK(fr::get_hashability<HVB::CommutativeTuple<>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::CommutativeTuple<int>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::CommutativeTuple<int, float>>()
			== HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::CommutativeTuple<int, UnhashableAggregate>>()
			== HA{Wrapper, None});

		STATIC_CHECK(fr::get_hashability<HVB::Range<int*, int*>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::Range<int*, int*, 5>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::Range<PrivateClass*, PrivateClass*>>()
			== HA{Wrapper, None});

		STATIC_CHECK(fr::get_hashability<HVB::CommutativeRange<int*, int*>>()
			== HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::CommutativeRange<PrivateClass*, PrivateClass*>>()
			== HA{Wrapper, None});

		STATIC_CHECK(fr::get_hashability<HVB::Optional<int>>() == HA{Wrapper, OptOut});
		STATIC_CHECK(fr::get_hashability<HVB::Optional<PrivateClass>>() == HA{Wrapper, None});
	}
	SECTION("Custom") {
		STATIC_CHECK(fr::get_hashability<CustomStruct>() == HA{Custom, OptOut});
	}
	SECTION("Compound") {
		STATIC_CHECK(fr::get_hashability<SimpleEnum>() == HA{Enum, AsBytes});

		STATIC_CHECK(fr::get_hashability<std::string>() == HA{String, OptOut});
		STATIC_CHECK(fr::get_hashability<std::u16string>() == HA{String, OptOut});
		STATIC_CHECK(fr::get_hashability<std::string_view>() == HA{String, OptOut});

		STATIC_CHECK(fr::get_hashability<std::optional<int>>() == HA{Optional, OptOut});
		STATIC_CHECK(fr::get_hashability<std::optional<PrivateClass>>() == HA{Optional, None});

		STATIC_CHECK(fr::get_hashability<int[5]>() == HA{Array, AsBytes});
		STATIC_CHECK(fr::get_hashability<PrivateClass[5]>() == HA{Array, None});

		STATIC_CHECK(fr::get_hashability<std::array<int, 5>>() == HA{Array, AsBytes});
		STATIC_CHECK(fr::get_hashability<std::array<PrivateClass, 5>>() == HA{Array, None});

		STATIC_CHECK(fr::get_hashability<std::vector<unsigned>>() == HA{Range, OptOut});
		STATIC_CHECK(fr::get_hashability<std::vector<AbstractClass>>() == HA{Range, None});
		STATIC_CHECK(fr::get_hashability<decltype(simple_view())>() == HA{Range, OptOut});
		STATIC_CHECK(fr::get_hashability<decltype(unhashable_view())>() == HA{Range, None});
	}
	SECTION("Described") {
		STATIC_CHECK(fr::get_hashability<DescribedOptOutClass>() == HA{Described, OptOut});
		STATIC_CHECK(fr::get_hashability<DescribedOptInClass>() == HA{Described, OptIn});
		STATIC_CHECK(fr::get_hashability<DescribedAsBytesClass>() == HA{Described, AsBytes});
		STATIC_CHECK(fr::get_hashability<DescribedHashableClass>() == HA{Described, OptOut});

		STATIC_CHECK(fr::get_hashability<DescribedNonHashableClass>() == HA{Described, None});
		STATIC_CHECK(fr::get_hashability<DescribedNoneClass>() == HA{Described, None});
	}
	SECTION("Record") {
		STATIC_CHECK(fr::get_hashability<HashableAggregate>() == HA{Record, OptOut});
		STATIC_CHECK(fr::get_hashability<ByteHashableAggregate>() == HA{Record, AsBytes});
		STATIC_CHECK(fr::get_hashability<PaddedHashableAggregate>() == HA{Record, OptOut});

		STATIC_CHECK(fr::get_hashability<UnhashableAggregate>() == HA{Record, None});
	}
	SECTION("Unhashable") {
		STATIC_CHECK(fr::get_hashability<AbstractClass>() == HA{Unhashable, None});
		STATIC_CHECK(fr::get_hashability<PrivateClass>() == HA{Unhashable, None});
	}
}

TEST_CASE("UniHasher.lenses1", "[u][engine][core][hashing]") {
	using L1 = fr::detail::UniHashableLens1;
	SECTION("one byte-hashable") {
		const auto lenses = fr::detail::UniHashableLenses1{fr::mp_list<char>};
		CHECK(lenses.byte_hashables() == std::vector<L1>{{{}, 1}});
	}
	SECTION("one other") {
		const auto lenses = fr::detail::UniHashableLenses1{fr::mp_list<long double>};
		CHECK(lenses.others() == std::vector<L1>{{{}, 0}});
	}
	SECTION("many") {
		const auto lenses = fr::detail::UniHashableLenses1{fr::mp_list<
			int32_t,
			long double,
			DescribedOptInClass,
			char,
			HashableAggregate,
			DescribedHashableWithBasesAndProps
		>};

		CHECK(lenses.byte_hashables().size() == 9);

		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{0}, 4}));
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{2, 0}, 4}));
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{3}, 1}));
		// Aggregate
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{4, 0}, 4}));
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{4, 2}, 16}));
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{4, 3, 1}, 1}));
		// WithBasesAndProps
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{5, 0}, 4})); // BaseA::a
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{5, 1, 0}, 2})); // BaseB::b
		CHECK(std::ranges::contains(lenses.byte_hashables(), L1{{5, 4}, 8})); // _z

		CHECK(lenses.others() == std::vector<L1>{
			{{1}, 0},
			{{2, 2}, 0},
			// Aggregate
			{{4, 1}, 0},
			{{4, 3, 0}, 0},
			{{4, 3, 2}, 0},
			// WithBasesAndProps
			{{5, 1, 1}, 0}, // BaseB::c
			{{5, 5}, 0} // y
		});
	}
}

namespace {

struct LensTester {
	template<auto Lens>
	auto get() const -> decltype(auto) {
		return fr::detail::apply_uni_hashable_lens<Lens>(a0, a1, a2, a3, a4, a5);
	}

public:
	int32_t a0 = 0;
	long double a1 = 1.l;
	DescribedOptInClass a2 {2, 200, "desc"};
	char a3 = '3';
	HashableAggregate a4 {4, "aggr", {43, 45}, {"t0", 46, "t2"}};
	DescribedHashableWithBasesAndProps a5{SimpleEnum::E5, 50, "bp", 530, "BP", 5020};
};

} // namespace

TEST_CASE("UniHasher.lenses2", "[u][engine][core][hashing]") {
	SECTION("one byte-hashable") {
		constexpr auto lenses2 = fr::detail::build_uni_hashable_lenses2(fr::mp_list<char>);
		CHECK(fr::detail::apply_uni_hashable_lens<lenses2.byte_hashables[0]>('a') == 'a');
	}
	SECTION("one other") {
		constexpr auto lenses2 = fr::detail::build_uni_hashable_lenses2(fr::mp_list<long double>);
		CHECK(fr::detail::apply_uni_hashable_lens<lenses2.others[0]>(1.l) == 1.l);
	}
	SECTION("many") {
		constexpr auto lenses2 = fr::detail::build_uni_hashable_lenses2(fr::mp_list<
			int32_t,
			long double,
			DescribedOptInClass,
			char,
			HashableAggregate,
			DescribedHashableWithBasesAndProps
		>);

		REQUIRE(lenses2.byte_hashables.size() == 9);
		REQUIRE(lenses2.others.size() == 7);

		const auto tester = LensTester{};
		// CHECK(tester.get<lenses2.byte_hashables[0]>() == 0);
		// CHECK(tester.get<lenses2.byte_hashables[1]>() == 2);
		// CHECK(tester.get<lenses2.byte_hashables[2]>() == '3');
		// CHECK(tester.get<lenses2.byte_hashables[3]>() == 4);
		// CHECK(tester.get<lenses2.byte_hashables[4]>() == std::array<int64_t, 2>{43, 45});
		// CHECK(tester.get<lenses2.byte_hashables[5]>() == 46);
		// CHECK(tester.get<lenses2.byte_hashables[6]>() == BaseA{SimpleEnum::E5});
		// CHECK(tester.get<lenses2.byte_hashables[7]>() == 50);
		// CHECK(tester.get<lenses2.byte_hashables[8]>() == 5020);

		CHECK(tester.get<lenses2.others[0]>() == 1.l);
		CHECK(tester.get<lenses2.others[1]>() == "desc");
		CHECK(tester.get<lenses2.others[2]>() == "aggr");
		CHECK(tester.get<lenses2.others[3]>() == "t0");
		CHECK(tester.get<lenses2.others[4]>() == "t2");
		CHECK(tester.get<lenses2.others[5]>() == "bp");
		CHECK(tester.get<lenses2.others[6]>() == "BP");
	}
}

// UniHasher tests
// ---------------

using ConstexprHashers = fr::MpList<
	fr::UniHasher<{64, false, Stable, 0, false}>,
	fr::UniHasher<{64, false, Stable, 321, false}>,
	fr::UniHasher<{64, false, Provided, 0, false}>,
	fr::UniHasher<{64, true, Stable, 0, false}>,
	fr::UniHasher<{64, true, Stable, 321, false}>,
	fr::UniHasher<{64, true, Provided, 0, false}>,
	fr::UniHasher<{64, true, Provided, 0, true}>,

	fr::UniHasher<{32, false, Stable, 0, false}>,
	fr::UniHasher<{32, false, Stable, 321, false}>,
	fr::UniHasher<{32, false, Provided, 0, false}>,
	fr::UniHasher<{32, true, Stable, 0, false}>,
	fr::UniHasher<{32, true, Stable, 321, false}>,
	fr::UniHasher<{32, true, Provided, 0, false}>,
	fr::UniHasher<{32, true, Provided, 0, true}>,

	fr::UniHasher<{16, false, Stable, 0, false}>,
	fr::UniHasher<{16, false, Stable, 321, false}>,
	fr::UniHasher<{16, false, Provided, 0, false}>,
	fr::UniHasher<{16, true, Stable, 0, false}>,
	fr::UniHasher<{16, true, Stable, 321, false}>,
	fr::UniHasher<{16, true, Provided, 0, false}>,
	fr::UniHasher<{16, true, Provided, 0, true}>
>;

// NOTE: No 16 bit hashers since the collision chance is way too high
using UnstableHashers = fr::MpList<
	fr::UniHasher<{64, false, Unstable, 0, false}>,
	fr::UniHasher<{64, true, Unstable, 0, false}>,
	fr::UniHasher<{64, true, Unstable, 0, true}>,

	fr::UniHasher<{32, false, Unstable, 0, false}>,
	fr::UniHasher<{32, true, Unstable, 0, false}>,
	fr::UniHasher<{32, true, Unstable, 0, true}>
>;

using Hashers = fr::MpConcat<ConstexprHashers, UnstableHashers>;

template<class Hasher>
static constexpr
auto make_hasher() -> Hasher {
	if constexpr (Hasher::opts.seeding == Provided)
		return Hasher{123456};
	else
		return Hasher{};
}

TEST_CASE("UIntOfSize", "[u][engine][core][hashing]") {
	STATIC_CHECK(std::same_as<fr::UIntOfSize<1>, uint8_t>);
	STATIC_CHECK(std::same_as<fr::UIntOfSize<2>, uint16_t>);
	STATIC_CHECK(std::same_as<fr::UIntOfSize<4>, uint32_t>);
	STATIC_CHECK(std::same_as<fr::UIntOfSize<8>, uint64_t>);
}

TEST_CASE("HashDigestOfSize", "[u][engine][core][hashing]") {
	STATIC_CHECK(std::same_as<fr::HashDigestOfSize<2>, fr::HashDigest16>);
	STATIC_CHECK(std::same_as<fr::HashDigestOfSize<4>, fr::HashDigest32>);
	STATIC_CHECK(std::same_as<fr::HashDigestOfSize<8>, fr::HashDigest64>);
}

TEST_CASE("UniHasher.SFINAE", "[u][engine][core][hashing]") {
	using HashableTypes = fr::MpList<
		bool,
		int,
		float,
		long double,
		std::nullptr_t,

		HVB::Digest<fr::HashDigest32>,
		HVB::Bytes<std::byte, size_t>,
		HVB::Bytes<char, int>,
		HVB::Bytes<unsigned char, long>,
		HVB::FixedBytes<std::byte, 12zu>,
		HVB::FixedBytes<char, 24>,
		HVB::FixedBytes<unsigned char, 50l>,
		HVB::String<char, size_t>,
		HVB::String<unsigned char, int>,
		HVB::String<char16_t, long>,
		HVB::FixedString<char, 12zu>,
		HVB::FixedString<unsigned char, 24>,
		HVB::FixedString<char16_t, 50l>,
		HVB::Ptr<int>,
		HVB::Ptr<CustomStruct>,
		HVB::Ptr<std::vector<int>>,
		HVB::Ptr<PrivateClass>,
		HVB::Tuple<int>,
		HVB::Tuple<int, int, bool, int>,
		HVB::Tuple<CustomStruct, float, std::string>,
		HVB::CommutativeTuple<CustomStruct, float, std::string>,
		HVB::Range<int*, int*>,
		HVB::Range<std::vector<int>::const_iterator, std::vector<int>::const_iterator>,
		HVB::CommutativeRange<int*, int*>,
		HVB::CommutativeRange< std::vector<int>::const_iterator, std::vector<int>::const_iterator>,
		HVB::Optional<std::string>,

		CustomStruct,

		SimpleEnum,
		DescribedOptOutClass,
		DescribedOptInClass,
		DescribedAsBytesClass,
		DescribedHashableClass,

		std::array<int, 5>,
		std::vector<int>,
		std::string,
		std::string_view,
		std::vector<CustomStruct>,
		std::optional<int>,
		std::optional<CustomStruct>,

		HashableAggregate
	>;

	using UnhashableTypes = fr::MpList<
		int*,

		HVB::Tuple<int, PrivateClass, PrivateClass>,
		HVB::CommutativeTuple<int, PrivateClass, PrivateClass>,
		HVB::Optional<PrivateClass>,

		std::unique_ptr<int>,
		std::any,
		std::optional<PrivateClass>,
		std::vector<PrivateClass>,

		DescribedNonHashableClass,
		DescribedNoneClass,

		AbstractClass,
		PrivateClass
	>;

	SECTION("c_hashable") {
		fr::for_each_type<HashableTypes>([]<class T> {
			STATIC_CHECK(fr::c_hashable<T>);
		});
		fr::for_each_type<UnhashableTypes>([]<class T> {
			STATIC_CHECK_FALSE(fr::c_hashable<T>);
		});
	}
	SECTION("c_hashable_by<UniHasher>") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			fr::for_each_type<HashableTypes>([]<class T> {
				STATIC_CHECK(fr::c_hashable_by<T, Hasher>);
			});

			fr::for_each_type<UnhashableTypes>([]<class T> {
				STATIC_CHECK_FALSE(fr::c_hashable_by<T, Hasher>);
			});
		});
	}
}

TEST_CASE("UniHasher.type-properties", "[u][engine][core][hashing]") {
	fr::for_each_type<Hashers>([]<class Hasher> {
		if constexpr (Hasher::opts.seeding != Provided) {
			STATIC_CHECK(std::is_default_constructible_v<Hasher>);
			STATIC_CHECK(std::is_nothrow_default_constructible_v<Hasher>);
			STATIC_CHECK(std::is_trivially_default_constructible_v<Hasher>);
		}

		STATIC_CHECK(std::is_copy_constructible_v<Hasher>);
		STATIC_CHECK(std::is_nothrow_copy_constructible_v<Hasher>);
		STATIC_CHECK(std::is_trivially_copy_constructible_v<Hasher>);

		STATIC_CHECK(std::is_copy_assignable_v<Hasher>);
		STATIC_CHECK(std::is_nothrow_copy_assignable_v<Hasher>);
		STATIC_CHECK(std::is_trivially_copy_assignable_v<Hasher>);

		STATIC_CHECK(std::is_move_constructible_v<Hasher>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<Hasher>);
		STATIC_CHECK(std::is_trivially_move_constructible_v<Hasher>);

		STATIC_CHECK(std::is_move_assignable_v<Hasher>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<Hasher>);
		STATIC_CHECK(std::is_trivially_move_assignable_v<Hasher>);

		STATIC_CHECK(std::is_destructible_v<Hasher>);
		STATIC_CHECK(std::is_nothrow_destructible_v<Hasher>);
		STATIC_CHECK(std::is_trivially_destructible_v<Hasher>);
	});
}

TEST_CASE("UniHasher.primitives", "[u][engine][core][hashing]") {
	fr::for_each_type<Hashers>([]<class Hasher> {
		INFO(fr::unqualified_type_name<Hasher>);
		const auto hasher = make_hasher<Hasher>();

		CHECK(hasher(true) != hasher(false));

		CHECK(hasher(23) != hasher(-23));
		CHECK(hasher(23u) != hasher(24u));

		CHECK(hasher(-0.f) == hasher(0.f));
		CHECK(hasher(1.f) != hasher(2.f));

		CHECK(hasher(-0.) == hasher(0.));
		CHECK(hasher(1.) != hasher(2.));

		CHECK(hasher(-0.l) == hasher(0.l));
		CHECK(hasher(1.l) != hasher(2.l));

		CHECK(hasher(nullptr) == hasher(nullptr));
	});
}

TEST_CASE("UniHasher.wrapper-Ptr", "[u][engine][core][hashing]") {
	fr::for_each_type<Hashers>([]<class Hasher> {
		INFO(fr::unqualified_type_name<Hasher>);
		const auto hasher = make_hasher<Hasher>();

		int x = 5;
		int y = 5;
		CHECK(hasher(HVB::ptr(&x)) != hasher(HVB::ptr(&y)));

		const void* ptr = nullptr;
		CHECK(hasher(HVB::ptr(ptr)) == hasher(nullptr));
	});
}

TEST_CASE("UniHasher.wrapper-Commutative", "[u][engine][core][hashing]") {
	fr::for_each_type<Hashers>([]<class Hasher> {
		INFO(fr::unqualified_type_name<Hasher>);
		const auto hasher = make_hasher<Hasher>();

		CHECK(hasher(HVB::commutative_tuple('a', 'b'))
			== hasher(HVB::commutative_tuple('b', 'a')));

		CHECK(hasher(HVB::commutative_tuple('a', 'b', 0xC0))
			== hasher(HVB::commutative_tuple('b', 0xC0, 'a')));
		CHECK(hasher(HVB::commutative_tuple('a', 'b', 0xC0, 0xD0))
			== hasher(HVB::commutative_tuple('a', 0xC0, 'b', 0xD0)));

		CHECK(hasher(HVB::tuple(2, HVB::commutative_tuple('a', 'b', 'c'), 3, 4))
			== hasher(HVB::tuple(2, HVB::commutative_tuple('b', 'c', 'a'), 3, 4)));

		CHECK(hasher(HVB::tuple(2, HVB::commutative_tuple('a', 'b', 'c'), 3, 4))
			!= hasher(HVB::tuple(2, HVB::commutative_tuple('b', 'c', 'a'), 4, 3)));

		CHECK(hasher(HVB::commutative_range(std::vector{'a', 'b', 'c'}))
			== hasher(HVB::commutative_range(std::vector{'c', 'a', 'b'})));

		CHECK(hasher(HVB::tuple(2, HVB::commutative_range(std::vector{'a', 'b', 'c'}), 3, 4))
			== hasher(HVB::tuple(2, HVB::commutative_range(std::vector{'b', 'c', 'a'}), 3, 4))
		);
	});
}

TEST_CASE("UniHasher.strings", "[u][engine][core][hashing]") {
	SECTION("non-equal objects => different hash codes") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			const auto hasher = make_hasher<Hasher>();

			CHECK(hasher("abc"s) == hasher("abc"s));
		});
	}
	SECTION("hash codes of string_view and string are equal") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			const auto hasher = make_hasher<Hasher>();

			CHECK(hasher("ABC"sv) == hasher("ABC"s));
			// NOTE: The following check fails because "ABC" gets hashed as an array `const char[4]`
			// CHECK(hasher("ABC"sv) == hasher("ABC"));
		});
	}

	SECTION("runtime hash code is equal to compile-time hash code") {
		fr::for_each_type<ConstexprHashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto comp_hasher = make_hasher<Hasher>();
			auto rt_hasher = make_hasher<Hasher>();

			SECTION("wstring") {
				constexpr auto comp_hash = comp_hasher(L"qwerty"s);
				auto rt_str = L"qwerty"s;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}
			SECTION("wstring_view") {
				constexpr auto comp_hash = comp_hasher(L"qwerty"sv);
				auto rt_str = L"qwerty"sv;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}

			SECTION("u8string") {
				constexpr auto comp_hash = comp_hasher(u8"qwerty"s);
				auto rt_str = u8"qwerty"s;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}
			SECTION("u8string_view") {
				constexpr auto comp_hash = comp_hasher(u8"qwerty"sv);
				auto rt_str = u8"qwerty"sv;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}

			SECTION("u16string") {
				constexpr auto comp_hash = comp_hasher(u"qwerty"s);
				auto rt_str = u"qwerty"s;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}
			SECTION("u16string_view") {
				constexpr auto comp_hash = comp_hasher(u"qwerty"sv);
				auto rt_str = u"qwerty"sv;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}

			SECTION("u32string") {
				constexpr auto comp_hash = comp_hasher(U"qwerty"s);
				auto rt_str = U"qwerty"s;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}
			SECTION("u32string_view") {
				constexpr auto comp_hash = comp_hasher(U"qwerty"sv);
				auto rt_str = U"qwerty"sv;
				auto rt_hash = rt_hasher(rt_str);
				CHECK(comp_hash == rt_hash);
			}
		});
	}
}

TEST_CASE("UniHasher.custom", "[u][engine][core][hashing]") {
	SECTION("equal objects => equal hash codes") {
		fr::for_each_type<ConstexprHashers>([]<class Hasher> {
			constexpr auto hasher = make_hasher<Hasher>();

			STATIC_CHECK(hasher(CustomStruct{2, 3, -11.f, true})
				== hasher(CustomStruct{2, 3, -11.f, true}));
		});
	}
	SECTION("non-equal objects => different hash codes") {
		fr::for_each_type<ConstexprHashers>([]<class Hasher> {
			constexpr auto hasher = make_hasher<Hasher>();

			STATIC_CHECK(hasher(CustomStruct{2, 3, -11.f, true})
				!= hasher(CustomStruct{2, 3, 11.f, true}));
			STATIC_CHECK(hasher(CustomStruct{2, 3, -11.f, true})
				!= hasher(CustomStruct{3, 2, -11.f, true}));
		});
	}
	SECTION("runtime hash code is equal to compile-time hash code") {
		fr::for_each_type<ConstexprHashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto comp_hasher = make_hasher<Hasher>();
			auto rt_hasher = make_hasher<Hasher>();

			constexpr auto comp_hash = comp_hasher(CustomStruct{2, 3, -11.f, true});
			auto rt_obj = CustomStruct{2, 3, -11.f, true};
			auto rt_hash = rt_hasher(rt_obj);
			CHECK(comp_hash == rt_hash);
		});
	}
}

TEST_CASE("UniHasher.described", "[u][engine][core][hashing]") {
	SECTION("OptOut") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = DescribedOptOutClass{1, 15, "aaaaa"};
			const auto b = DescribedOptOutClass{2, 15, "bbbbb"};
			const auto c = DescribedOptOutClass{1, 15, "ccccc"};

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("OptIn") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = DescribedOptInClass{1, 15, "aaaaa"};
			const auto b = DescribedOptInClass{1, 15, "bbbbb"};
			const auto c = DescribedOptInClass{1, 35, "aaaaa"};

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("AsBytes") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = DescribedAsBytesClass{1, 15, 18};
			const auto b = DescribedAsBytesClass{1, 15, 24};
			const auto c = DescribedAsBytesClass{1, 15, 18};

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("Hashable{true}") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = DescribedHashableClass{1, 15, "aaa"};
			const auto b = DescribedHashableClass{1, 15, "bbb"};
			const auto c = DescribedHashableClass{1, 15, "aaa"};

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("WithBasesAndProps{true}") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();
			using enum SimpleEnum;

			const auto a = DescribedHashableWithBasesAndProps{E1, 15, "aaa", 1, "yy", 55};
			const auto b = DescribedHashableWithBasesAndProps{E1, 15, "aaa", 2, "yy", 55};
			const auto c = DescribedHashableWithBasesAndProps{E3, 15, "aaa", 1, "yy", 55};
			const auto d = DescribedHashableWithBasesAndProps{E1, 24, "ddd", 1, "yy", 55};
			const auto e = DescribedHashableWithBasesAndProps{E1, 15, "aaa", 1, "Y", 55};
			const auto f = DescribedHashableWithBasesAndProps{E1, 15, "aaa", 1, "yy", 60};

			CHECK(hasher(a) == hasher(b));
			CHECK(hasher(a) != hasher(c));
			CHECK(hasher(a) != hasher(d));
			CHECK(hasher(a) != hasher(e));
			CHECK(hasher(a) != hasher(f));
		});
	}
}

TEST_CASE("UniHasher.records", "[u][engine][core][hashing]") {
	SECTION("HashableAggregate") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = HashableAggregate{10, "aaa", {25, 35}, {"a0", 12, "a2"}};
			const auto b = HashableAggregate{10, "aaa", {35, 25}, {"b0", 13, "b2"}};
			const auto c = HashableAggregate{10, "aaa", {25, 35}, {"a0", 12, "a2"}};

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("ByteHashableAggregate") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = ByteHashableAggregate{10, -34, -56};
			const auto b = ByteHashableAggregate{10, -55, -56};
			const auto c = ByteHashableAggregate{10, -34, -56};

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("PaddedHashableAggregate") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = PaddedHashableAggregate{10, 34};
			const auto b = PaddedHashableAggregate{34, 34};
			const auto c = PaddedHashableAggregate{10, 34};

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("std::tuple") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = std::make_tuple(10, 34, 53);
			const auto b = std::make_tuple(10, 20, 53);
			const auto c = std::make_tuple(10, 34, 53);

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
	SECTION("std::pair") {
		fr::for_each_type<Hashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto hasher = make_hasher<Hasher>();

			const auto a = std::make_pair(10, 34);
			const auto b = std::make_pair(10, 20);
			const auto c = std::make_pair(10, 34);

			CHECK(hasher(a) != hasher(b));
			CHECK(hasher(a) == hasher(c));
		});
	}
}

TEST_CASE("UniHasher.enum", "[u][engine][core][hashing]") {
	fr::for_each_type<Hashers>([]<class Hasher> {
		INFO(fr::unqualified_type_name<Hasher>);
		constexpr auto hasher = make_hasher<Hasher>();

		const auto a = SimpleEnum::E1;
		const auto b = SimpleEnum::E0;
		const auto c = SimpleEnum::E1;

		CHECK(hasher(a) != hasher(b));
		CHECK(hasher(a) == hasher(c));
	});
}

TEST_CASE("UniHasher.optional", "[u][engine][core][hashing]") {
	fr::for_each_type<ConstexprHashers>([]<class Hasher> {
		INFO(fr::unqualified_type_name<Hasher>);
		constexpr auto comp_hasher = make_hasher<Hasher>();
		auto rt_hasher = make_hasher<Hasher>();

		constexpr auto get_a = [] -> std::optional<std::string> { return "abc"; };
		constexpr auto get_b = [] -> std::optional<std::string> { return "qwerty"; };
		constexpr auto get_c = [] -> std::optional<std::string> { return "abc"; };
		constexpr auto get_d = [] -> std::optional<std::string> { return std::nullopt; };

		CHECK(comp_hasher(get_a()) != comp_hasher(get_b()));
		CHECK(comp_hasher(get_a()) == comp_hasher(get_c()));
		CHECK(comp_hasher(get_a()) != comp_hasher(get_d()));

		constexpr auto comp_hash = comp_hasher(get_a());
		auto rt_hash = rt_hasher(get_a());
		CHECK(comp_hash == rt_hash);
	});
}

TEST_CASE("UniHasher.range", "[u][engine][core][hashing]") {
	SECTION("CustomStruct[4]") {
		fr::for_each_type<ConstexprHashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto comp_hasher = make_hasher<Hasher>();
			auto rt_hasher = make_hasher<Hasher>();

			constexpr CustomStruct vec_a[] = {
				{10, 1, -0.3f, true},
				{24, 2, +3.5f, false},
				{32, 3, +2.1f, false},
				{49, 4, -9.9f, true},
			};
			constexpr CustomStruct vec_b[] = {
				{10, 1, -0.3f, true},
				{24, 2, +3.5f, false},
				{32, 3, +2.1f, false},
				{49, 4, -9.9f, false},
			};

			STATIC_CHECK(comp_hasher(vec_a) != comp_hasher(vec_b));

			constexpr auto comp_hash = comp_hasher(vec_a);
			auto rt_hash = rt_hasher(vec_a);
			CHECK(comp_hash == rt_hash);
		});
	}
	SECTION("std::vector<ByteHashableAggregate>") {
		fr::for_each_type<ConstexprHashers>([]<class Hasher> {
			INFO(fr::unqualified_type_name<Hasher>);
			constexpr auto comp_hasher = make_hasher<Hasher>();
			auto rt_hasher = make_hasher<Hasher>();

			constexpr auto get_vec_a = [] -> std::vector<ByteHashableAggregate> {
				return {
					{10, 24, 45},
					{59, 22, 22},
					{42, -3, -5},
				};
			};
			constexpr auto get_vec_b = [] -> std::vector<ByteHashableAggregate> {
				return {
					{10, 24, 45},
					{59, 22, 22}
				};
			};

			STATIC_CHECK(comp_hasher(get_vec_a()) != comp_hasher(get_vec_b()));

			constexpr auto comp_hash = comp_hasher(get_vec_a());
			auto rt_hash = rt_hasher(get_vec_a());
			CHECK(comp_hash == rt_hash);
		});
	}
}

TEST_CASE("UniHasher.provided-static-seed", "[u][engine][core][hashing]") {
	using HasherA = fr::UniHasher<{.seeding = Stable, .seed = 25}>;
	using HasherB = fr::UniHasher<{.seeding = Stable, .seed = 46}>;
	using HasherC = fr::UniHasher<{.seeding = Stable, .seed = 25}>;

	constexpr auto hasher_a = HasherA{};
	constexpr auto hasher_b = HasherB{};
	constexpr auto hasher_c = HasherC{};

	STATIC_CHECK(hasher_a.seed() == 25);
	STATIC_CHECK(hasher_b.seed() == 46);
	STATIC_CHECK(hasher_c.seed() == 25);

	STATIC_CHECK(hasher_a("abc") == hasher_c("abc"));
	STATIC_CHECK(hasher_a(123u) == hasher_c(123u));

	STATIC_CHECK(hasher_a("abc") != hasher_b("abc"));
	STATIC_CHECK(hasher_a(123u) != hasher_b(123u));
	STATIC_CHECK(hasher_a(46) != hasher_b(25));
}

TEST_CASE("UniHasher.provided-seed", "[u][engine][core][hashing]") {
	using Hasher = fr::UniHasher<{.seeding = Provided}>;

	const auto hasher_a = Hasher{25};
	auto hasher_b = Hasher{46};
	const auto hasher_c = Hasher{25};

	CHECK(hasher_a.seed() == 25);
	CHECK(hasher_b.seed() == 46);
	CHECK(hasher_c.seed() == 25);

	CHECK(hasher_a("abc") == hasher_c("abc"));
	CHECK(hasher_a(123u) == hasher_c(123u));

	CHECK(hasher_a("abc") != hasher_b("abc"));
	CHECK(hasher_a(123u) != hasher_b(123u));
	CHECK(hasher_a(46) != hasher_b(25));

	hasher_b.reseed(25);
	CHECK(hasher_b.seed() == 25);

	CHECK(hasher_a("abc") == hasher_b("abc"));
	CHECK(hasher_a(123u) == hasher_b(123u));
}
