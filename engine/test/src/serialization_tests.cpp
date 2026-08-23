#include "fractal_box/core/serialization/sbs_data_format.hpp"
#include "fractal_box/core/serialization/serialization_concepts.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/array_utils.hpp"
#include "fractal_box/core/io/span_io.hpp"
#include "fractal_box/core/io/vector_io.hpp"
#include "fractal_box/core/string_utils.hpp"

#include "test_common/test_helpers.hpp"

namespace {

constexpr
auto serialized_size(fr::c_arithmetic auto value) noexcept -> size_t {
	return sizeof(value);
}

constexpr
auto serialized_size(fr::c_enum auto value) noexcept -> size_t {
	return sizeof(value);
}

constexpr
auto serialized_size(const std::string& value) noexcept -> size_t {
	return sizeof(size_t) + value.size();
}

constexpr
auto serialized_size(const fr::c_array_like auto& arr) noexcept -> size_t;

template<class T, class U>
constexpr
auto serialized_size(const std::pair<T, U>& pair) noexcept -> size_t {
	return serialized_size(pair.first) + serialized_size(pair.second);
}

template<class... Ts>
constexpr
auto serialized_size(const std::tuple<Ts...>& tuple) noexcept -> size_t {
	return std::apply([](const auto&... elems) {
		return (0zu + ... + serialized_size(elems));
	}, tuple);
}

constexpr
auto serialized_size(const fr::c_array_like auto& arr) noexcept -> size_t {
	auto sum = 0zu;
	for (const auto& v : arr)
		sum += serialized_size(v);
	return sum;
}

constexpr
auto serialized_size(const fr::c_dynamically_sized_range auto& arr) noexcept -> size_t {
	auto sum = sizeof(size_t);
	for (const auto& v : arr)
		sum += serialized_size(v);
	return sum;
}

template<class... Ts>
requires (sizeof...(Ts) > 1zu)
constexpr
auto serialized_size(const Ts&... values) noexcept -> size_t {
	return (0zu + ... + serialized_size(values));
}

struct FriendCustomizedStruct {
	auto operator==(const FriendCustomizedStruct&) const -> bool = default;

	template<class Ar>
	friend constexpr
	auto fr_custom_serialize(
		Ar& archive,
		fr::AddConstIf<FriendCustomizedStruct, Ar::is_encoding>& self
	) {
		return archive(self.x, self.y);
	}

	friend constexpr
	auto serialized_size(const FriendCustomizedStruct& self) noexcept -> size_t {
		return serialized_size(self.x, self.y);
	}

public:
	int x;
	std::string y;
};

struct StaticCustomizedStruct {
	auto operator==(const StaticCustomizedStruct&) const -> bool = default;

	static constexpr
	auto fr_custom_serialize(auto& archive, auto& self) {
		return archive(self.x, self.y);
	}

	friend constexpr
	auto serialized_size(const StaticCustomizedStruct& self) noexcept -> size_t {
		return serialized_size(self.x, self.y);
	}

public:
	int x;
	std::string y;
};

class AbstractClass {
public:
	virtual ~AbstractClass() = 0;
};

class PrivateClass {
public:
	auto operator==(const PrivateClass&) const noexcept -> bool = default;

	auto foo() const noexcept -> int { return _foo; }

private:
	[[maybe_unused]]
	int _foo;
};

} // namespace

template<>
struct std::hash<PrivateClass> {
	auto operator()(const PrivateClass& self) const noexcept -> size_t {
		return std::hash<decltype(self.foo())>{}(self.foo());
	}
};

namespace {

struct NoDefaultCtor {
	NoDefaultCtor() = delete;
	NoDefaultCtor(int) { }
};

enum class SimpleEnum {
	A,
	B
};

struct SimpleAggregate {
	auto operator==(const SimpleAggregate&) const -> bool = default;

	friend constexpr
	auto serialized_size(const SimpleAggregate& self) noexcept -> size_t {
		return serialized_size(self.x, self.y);
	}

public:
	float x;
	int64_t y;
};

struct ComplexAggregate {
	auto operator==(const ComplexAggregate&) const -> bool = default;

	friend constexpr
	auto serialized_size(const ComplexAggregate& self) noexcept -> size_t {
		return serialized_size(self.x, self.y, self.z);
	}

public:
	FriendCustomizedStruct x;
	SimpleAggregate y;
	std::array<int16_t, 3> z;
};

struct AggregateWithUnserializableField {
	int a;
	PrivateClass b;
};

struct DescribedIgnorantClass {
	using Self = DescribedIgnorantClass;

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) {
		return fr::class_desc<
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z>
		>;
	}

public: // NOTE: Make this also an aggregate to see if serializer respects description
	int _x {};
	int _y {};
	std::string _z;
};

struct DescribedOptOutClass {
	using Self = DescribedOptOutClass;

	DescribedOptOutClass() = default;

	constexpr
	DescribedOptOutClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	auto operator==(const Self&) const -> bool = default;

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::SerializableMode::OptOut>,
			fr::Field<&Self::_x, fr::Attributes<fr::Serializable{}>>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z, fr::Attributes<fr::Serializable{false}>>
		>;
	}

	friend constexpr
	auto serialized_size(const Self& self) noexcept {
		return serialized_size(self._x, self._y);
	}

private:
	int _x {};
	int _y {};
	std::string _z;
};

struct DescribedOptInClass {
	using Self = DescribedOptInClass;

	DescribedOptInClass() = default;

	constexpr
	DescribedOptInClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	auto operator==(const Self&) const -> bool = default;

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::SerializableMode::OptIn>,
			fr::Field<&Self::_x, fr::Attributes<fr::Serializable{}>>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z, fr::Attributes<fr::Serializable{true}>>
		>;
	}

	friend constexpr
	auto serialized_size(const Self& self) noexcept {
		return serialized_size(self._x, self._z);
	}

private:
	int _x {};
	int _y {};
	std::string _z;
};

struct DescribedNoneClass {
	using Self = DescribedNoneClass;

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::SerializableMode::None>,
			fr::Field<&Self::_x>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z>
		>;
	}

public: // NOTE: Make this also an aggregate to see if serializer respects description
	int _x {};
	int _y {};
	std::string _z;
};

struct DescribedSerializableClass {
	using Self = DescribedSerializableClass;

	DescribedSerializableClass() = default;

	constexpr
	DescribedSerializableClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	auto operator==(const Self&) const -> bool = default;

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::Serializable{}>,
			fr::Field<&Self::_x, fr::Attributes<fr::Serializable{}>>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z, fr::Attributes<fr::Serializable{false}>>
		>;
	}

	friend constexpr
	auto serialized_size(const Self& self) noexcept {
		return serialized_size(self._x, self._y);
	}

private:
	int _x {};
	int _y {};
	std::string _z;
};

struct DescribedSerializableFalseClass {
	using Self = DescribedSerializableFalseClass;

	DescribedSerializableFalseClass() = default;

	constexpr
	DescribedSerializableFalseClass(int x, int y, std::string z): _x{x}, _y{y}, _z(std::move(z)) { }

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) {
		return fr::class_desc<
			fr::Attributes<fr::Serializable{false}>,
			fr::Field<&Self::_x, fr::Attributes<fr::Serializable{}>>,
			fr::Field<&Self::_y>,
			fr::Field<&Self::_z, fr::Attributes<fr::Serializable{false}>>
		>;
	}

private:
	int _x {};
	int _y {};
	std::string _z;
};

struct BaseA {
	[[maybe_unused]] constexpr
	auto operator==(const BaseA&) const -> bool = default;

public:
	SimpleEnum a;
};

struct BaseB {
	[[maybe_unused]] constexpr
	auto operator==(const BaseB&) const -> bool = default;

	[[maybe_unused]] friend consteval
	auto fr_describe(const BaseB&) {
		return fr::class_desc<
			fr::Attributes<fr::Serializable{}>,
			fr::Field<&BaseB::b>,
			fr::Field<&BaseB::c>
		>;
	}

	friend constexpr
	auto serialized_size(const BaseB& self) noexcept {
		return serialized_size(self.b, self.c);
	}

public:
	int16_t b {};
	std::string c;
};

struct BaseC {
	[[maybe_unused]] constexpr
	auto operator==(const BaseC&) const -> bool = default;

public:
	float* ptr = nullptr;
};

struct DescribedWithBasesAndProps: public BaseA, public BaseB, public BaseC {
	using Self = DescribedWithBasesAndProps;

	DescribedWithBasesAndProps() = default;

	constexpr
	DescribedWithBasesAndProps(
		SimpleEnum a_val,
		int16_t b_val,
		std::string c_val,
		int x,
		std::string y,
		double z
	):
		BaseA{a_val},
		BaseB{b_val, std::move(c_val)},
		_x{x},
		_y{std::move(y)},
		_z{z}
	{ }

	auto operator==(const Self&) const -> bool = default;

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) {
		return fr::class_desc<
			fr::Bases<BaseA, BaseB, BaseC>,
			fr::Attributes<fr::SerializableMode::OptIn>,
			fr::Field<&Self::_x>,
			fr::Property<"y", std::string, &Self::y, &Self::set_y,
				fr::Attributes<fr::Serializable{}>>,
			fr::Field<&Self::_z, fr::Attributes<fr::Serializable{}>>
		>;
	}

	friend constexpr
	auto serialized_size(const Self& self) noexcept {
		return serialized_size(self.a, static_cast<const BaseB&>(self), self._y, self._z);
	}

	/// @note Returns by value to simulate properties calculated on the fly
	constexpr
	auto y() const -> std::string { return _y; }

	constexpr
	void set_y(std::string value) { _y = std::move(value); }

private:
	int _x {};
	std::string _y;
	double _z {};
};

template<
	bool IsCompTestEnabled = true,
	std::invocable<> Factory1,
	std::invocable<> Factory2,
	fr::c_size_c FallbackSize1 = fr::SizeC<0zu>,
	fr::c_size_c FallbackSize2 = fr::SizeC<0zu>
>
requires (!fr::c_size_c<Factory2>)
static constexpr
auto test_common_serde_scenarios(
	const std::string& name,
	Factory1,
	Factory2,
	FallbackSize1 forced_size1 = {},
	FallbackSize2 fallback_size2 = {}
) {
	using ValueType = decltype(Factory1::operator()());
	static_assert(std::is_same_v<decltype(Factory2::operator()()), ValueType>);
	static constexpr auto value1_size = [&] -> size_t {
		if constexpr (forced_size1 != 0zu) {
			static_assert(fallback_size2 != 0zu);
			return forced_size1;
		}
		else {
			return serialized_size(Factory1::operator()());
		}
	}();
	static constexpr auto value2_size = [&] -> size_t {
		if constexpr (fallback_size2 != 0zu) {
			return fallback_size2;
		}
		else {
			return serialized_size(Factory2::operator()());
		}
	}();

	constexpr auto run_tests = [] {
		frt::double_test<IsCompTestEnabled>("serializing into a vector", [] {
			const auto in_value1 = Factory1::operator()();
			const auto in_value2 = Factory2::operator()();

			auto buf = std::vector<unsigned char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value1) == value1_size);
			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value2) == value2_size);

			auto out_value1 = ValueType{};
			auto out_value2 = ValueType{};
			auto reader = fr::SpanReader{buf};

			auto res1 = fr::SbsDataFormat::decode(reader, out_value1);
			FRT_REQUIRE(res1);
			FRT_CHECK(*res1 == value1_size);
			FRT_CHECK(out_value1 == in_value1);

			auto res2 = fr::SbsDataFormat::decode(reader, out_value2);
			FRT_REQUIRE(res2);
			FRT_CHECK(*res2 == value2_size);
			FRT_CHECK(out_value2 == in_value2);
		});
		frt::double_test<IsCompTestEnabled>("serialializing into an array which is too small", [] {
			const auto in_value = Factory1::operator()();

			static_assert(value1_size > 2zu);
			auto buf = std::array<std::byte, value1_size - 2zu>{};
			auto writer = fr::SpanWriter{buf};

			auto res = fr::SbsDataFormat::encode(writer, in_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());
		});
		frt::double_test<IsCompTestEnabled>("deserializing from a span which is too small", [] {
			const auto in_value = Factory1::operator()();

			auto buf = std::vector<char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

			auto out_value = ValueType{};

			auto reader1 = fr::SpanReader{std::span<char>(buf.data(), buf.data() + 3zu)};
			auto res1 = fr::SbsDataFormat::decode(reader1, out_value);
			FRT_CHECK(!res1);
			FRT_CHECK(res1.template has_error<fr::BufferOverrun>());

			auto reader2 = fr::SpanReader{std::span<char>(buf.data(), buf.data() + value1_size
				- 2)};
			auto res2 = fr::SbsDataFormat::decode(reader2, out_value);
			FRT_CHECK(!res2);
			FRT_CHECK(res2.template has_error<fr::BufferOverrun>());
		});
	};
	if (name.empty()) {
		run_tests();
	}
	else {
		INFO(name);
		run_tests();
	}
}

template<
	bool IsCompTestEnabled = true,
	std::invocable<> Factory,
	fr::c_size_c FallbackSize = fr::SizeC<0zu>
>
static constexpr
auto test_common_serde_scenarios(
	const std::string& name,
	Factory,
	FallbackSize fallback_size = {}
) {
	return test_common_serde_scenarios<IsCompTestEnabled>(name, Factory{}, Factory{},
		fallback_size, fallback_size);
}

} // namespace

TEST_CASE("get_serializability", "[u][engine][core][serialization]") {
	using enum fr::SerializableMode;
	using enum fr::SerializableCategory;
	using SA = fr::Serializability;

	SECTION("Unserializable") {
		STATIC_CHECK(fr::get_serializability<AbstractClass>() == SA{Unserializable, None});
		STATIC_CHECK(fr::get_serializability<PrivateClass>() == SA{Unserializable, None});
		STATIC_CHECK(fr::get_serializability<int*>() == SA{Unserializable, None});
		STATIC_CHECK(fr::get_serializability<void (*)(int)>() == SA{Unserializable, None});
	}
	SECTION("Primitive") {
		STATIC_CHECK(fr::get_serializability<bool>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<signed char>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<char>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<short>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<int>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<unsigned long>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<float>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<long double>() == SA{Primitive, Default});
		STATIC_CHECK(fr::get_serializability<void>() == SA{Primitive, None});
		STATIC_CHECK(fr::get_serializability<std::nullptr_t>() == SA{Primitive, None});
	}
	SECTION("Custom") {
		STATIC_CHECK(fr::get_serializability<FriendCustomizedStruct>() == SA{Custom, Default});
		STATIC_CHECK(fr::get_serializability<StaticCustomizedStruct>() == SA{Custom, Default});
	}
	SECTION("Described") {
		STATIC_CHECK(fr::get_serializability<DescribedIgnorantClass>() == SA{Described, None});
		STATIC_CHECK(fr::get_serializability<DescribedOptOutClass>() == SA{Described, OptOut});
		STATIC_CHECK(fr::get_serializability<DescribedOptInClass>() == SA{Described, OptIn});
		STATIC_CHECK(fr::get_serializability<DescribedNoneClass>() == SA{Described, None});
		STATIC_CHECK(fr::get_serializability<DescribedSerializableClass>()
			== SA{Described, OptOut});
		STATIC_CHECK(fr::get_serializability<DescribedSerializableFalseClass>()
			== SA{Described, None});
		STATIC_CHECK(fr::get_serializability<DescribedWithBasesAndProps>() == SA{Described, OptIn});
	}
	SECTION("Enum") {
		STATIC_CHECK(fr::get_serializability<SimpleEnum>() == SA{Enum, Default});
	}
	SECTION("Optional") {
		STATIC_CHECK(fr::get_serializability<std::optional<int>>() == SA{Optional, Default});
		STATIC_CHECK(fr::get_serializability<std::optional<FriendCustomizedStruct>>()
			== SA{Optional, Default});

		STATIC_CHECK(fr::get_serializability<std::optional<PrivateClass>>()
			== SA{Optional, None});
		STATIC_CHECK(fr::get_serializability<std::optional<NoDefaultCtor>>()
			== SA{Optional, None});
	}
	SECTION("String") {
		STATIC_CHECK(fr::get_serializability<std::string>() == SA{String, Default});
		STATIC_CHECK(fr::get_serializability<std::u16string>() == SA{String, Default});
		STATIC_CHECK(fr::get_serializability<std::u32string>() == SA{String, Default});
		STATIC_CHECK(fr::get_serializability<std::wstring>() == SA{String, Default});
	}
	SECTION("Array") {
		STATIC_CHECK(fr::get_serializability<std::array<int, 5>>() == SA{Array, Default});
		STATIC_CHECK(fr::get_serializability<int[5]>() == SA{Array, Default});
		STATIC_CHECK(fr::get_serializability<int[5][3]>() == SA{Array, Default});

		STATIC_CHECK(fr::get_serializability<std::array<PrivateClass, 5>>() == SA{Array, None});
		STATIC_CHECK(fr::get_serializability<PrivateClass[5]>() == SA{Array, None});
	}
	SECTION("Vector") {
		STATIC_CHECK(fr::get_serializability<std::vector<int>>() == SA{Vector, Default});
		STATIC_CHECK(fr::get_serializability<std::vector<FriendCustomizedStruct>>()
			== SA{Vector, Default});

		STATIC_CHECK(fr::get_serializability<std::vector<PrivateClass>>() == SA{Vector, None});
		STATIC_CHECK(fr::get_serializability<std::vector<NoDefaultCtor>>() == SA{Vector, None});
	}
	SECTION("Map") {
		STATIC_CHECK(fr::get_serializability<std::map<int, std::string>>() == SA{Map, Default});
		STATIC_CHECK(fr::get_serializability<std::multimap<int, std::string>>()
			== SA{Map, Default});
		STATIC_CHECK(fr::get_serializability<std::unordered_map<int, std::string>>()
			== SA{Map, Default});
		STATIC_CHECK(fr::get_serializability<std::unordered_multimap<int, std::string>>()
			== SA{Map, Default});

		STATIC_CHECK(fr::get_serializability<std::map<PrivateClass, std::string>>()
			== SA{Map, None});
		STATIC_CHECK(fr::get_serializability<std::multimap<PrivateClass, std::string>>()
			== SA{Map, None});
		STATIC_CHECK(fr::get_serializability<std::unordered_map<PrivateClass, std::string>>()
			== SA{Map, None});
		STATIC_CHECK(fr::get_serializability<std::unordered_multimap<PrivateClass, std::string>>()
			== SA{Map, None});
		STATIC_CHECK(fr::get_serializability<std::map<int, PrivateClass>>() == SA{Map, None});
		STATIC_CHECK(fr::get_serializability<std::multimap<int, PrivateClass>>()
			== SA{Map, None});
		STATIC_CHECK(fr::get_serializability<std::unordered_map<int, PrivateClass>>()
			== SA{Map, None});
		STATIC_CHECK(fr::get_serializability<std::unordered_multimap<int, PrivateClass>>()
			== SA{Map, None});
	}
	SECTION("Set") {
		STATIC_CHECK(fr::get_serializability<std::set<int>>() == SA{Set, Default});
		STATIC_CHECK(fr::get_serializability<std::multiset<int>>() == SA{Set, Default});
		STATIC_CHECK(fr::get_serializability<std::unordered_set<int>>() == SA{Set, Default});
		STATIC_CHECK(fr::get_serializability<std::unordered_multiset<int>>() == SA{Set, Default});

		STATIC_CHECK(fr::get_serializability<std::set<PrivateClass>>() == SA{Set, None});
		STATIC_CHECK(fr::get_serializability<std::multiset<PrivateClass>>()
			== SA{Set, None});
		STATIC_CHECK(fr::get_serializability<std::unordered_set<PrivateClass>>()
			== SA{Set, None});
		STATIC_CHECK(fr::get_serializability<std::unordered_multiset<PrivateClass>>()
			== SA{Set, None});
	}
	SECTION("Variant") {
		STATIC_CHECK(fr::get_serializability<std::variant<int>>() == SA{Variant, Default});
		STATIC_CHECK(fr::get_serializability<std::variant<int, std::monostate>>()
			== SA{Variant, Default});
		STATIC_CHECK(fr::get_serializability<std::variant<int, FriendCustomizedStruct, float>>()
			== SA{Variant, Default});

		STATIC_CHECK(fr::get_serializability<std::variant<std::nullptr_t>>()
			== SA{Variant, None});
		STATIC_CHECK(fr::get_serializability<std::variant<int, PrivateClass, float>>()
			== SA{Variant, None});
	}
	SECTION("Record") {
		STATIC_CHECK(fr::get_serializability<std::monostate>() == SA{Record, Default});
		STATIC_CHECK(fr::get_serializability<SimpleAggregate>() == SA{Record, Default});
		STATIC_CHECK(fr::get_serializability<ComplexAggregate>() == SA{Record, Default});
		STATIC_CHECK(fr::get_serializability<std::tuple<int, std::string>>()
			== SA{Record, Default});
		STATIC_CHECK(fr::get_serializability<std::pair<int, std::string>>() == SA{Record, Default});

		STATIC_CHECK(fr::get_serializability<std::tuple<int, PrivateClass>>() == SA{Record, None});
		STATIC_CHECK(fr::get_serializability<AggregateWithUnserializableField>()
			== SA{Record, None});
	}
}

TEST_CASE("Serialization-concepts", "[u][engine][core][serialization]") {
	SECTION("c_data_format") {
		// TODO: Negative cases
		STATIC_CHECK(fr::c_data_format<fr::SbsDataFormat>);
	}
	SECTION("c_has_custom_serialize") {
		// TODO: Negative cases
		STATIC_CHECK(fr::c_has_custom_serialize<FriendCustomizedStruct>);
		STATIC_CHECK(fr::c_has_custom_serialize<StaticCustomizedStruct>);
	}

	using SerializableTypes = fr::MpTypes<
		bool,
		short,
		int,
		int&,
		unsigned long,
		float,
		long double,
		FriendCustomizedStruct,
		StaticCustomizedStruct,
		StaticCustomizedStruct&,
		SimpleEnum,
		SimpleEnum&,
		std::optional<int>,
		std::optional<int>&,
		std::optional<FriendCustomizedStruct>,
		std::string,
		std::string&,
		std::u16string,
		std::u32string,
		std::wstring,
		std::array<int, 5>,
		int[5],
		int(&)[5],
		int[5][3],
		std::vector<int>,
		std::vector<std::string>,
		std::vector<int>&,
		std::vector<FriendCustomizedStruct>,
		std::map<std::string, int>,
		std::multimap<std::string, int>,
		std::unordered_map<std::string, int>,
		std::unordered_multimap<std::string, int>,
		std::set<std::string>,
		std::multiset<std::string>,
		std::unordered_set<std::string>,
		std::unordered_multiset<std::string>,
		std::variant<int>,
		std::variant<int>&,
		std::variant<int, std::monostate>,
		std::variant<int, FriendCustomizedStruct, float>,
		std::monostate,
		SimpleAggregate,
		ComplexAggregate,
		std::tuple<int, std::string, StaticCustomizedStruct>,
		std::pair<int, StaticCustomizedStruct>
	>;

	using UnserializableTypes = fr::MpTypes<
		void,
		std::nullptr_t,
		int*,
		auto (char) -> int,
		auto (*)(char) -> int,
		auto (&)(char) -> int,
		AbstractClass,
		std::optional<PrivateClass>,
		std::optional<NoDefaultCtor>,
		std::array<PrivateClass, 5>,
		PrivateClass[5],
		std::vector<PrivateClass>,
		std::vector<NoDefaultCtor>,
		std::map<std::string, PrivateClass>,
		std::multimap<std::string, PrivateClass>,
		std::unordered_map<std::string, PrivateClass>,
		std::unordered_multimap<std::string, PrivateClass>,
		std::map<PrivateClass, int>,
		std::multimap<PrivateClass, int>,
		std::unordered_map<PrivateClass, int>,
		std::unordered_multimap<PrivateClass, int>,
		std::set<PrivateClass>,
		std::multiset<PrivateClass>,
		std::unordered_set<PrivateClass>,
		std::unordered_multiset<PrivateClass>,
		std::variant<std::nullptr_t>,
		std::variant<int, PrivateClass, float>,
		std::tuple<int, PrivateClass>,
		AggregateWithUnserializableField
	>;

	SECTION("c_serializable") {
		fr::for_each_type<SerializableTypes>([]<class T> {
			STATIC_CHECK(fr::c_serializable<T>);
		});
		fr::for_each_type<UnserializableTypes>([]<class T> {
			STATIC_CHECK_FALSE(fr::c_serializable<T>);
		});
	}
	SECTION("c_serializable_by") {
		fr::for_each_type<SerializableTypes>([]<class T> {
			STATIC_CHECK(fr::c_serializable_by<T, fr::SbsDataFormat>);
		});
		fr::for_each_type<UnserializableTypes>([]<class T> {
			STATIC_CHECK_FALSE(fr::c_serializable_by<T, fr::SbsDataFormat>);
		});
	}
	STATIC_CHECK(fr::c_serializable<int>);
	STATIC_CHECK(fr::c_serializable<FriendCustomizedStruct>);
	STATIC_CHECK(fr::c_serializable<StaticCustomizedStruct>);
	STATIC_CHECK(fr::c_serializable<std::string>);
	STATIC_CHECK(fr::c_serializable<std::vector<std::string>>);
}

TEST_CASE("SbsDataFormat.fundamentals", "[u][engine][core][serialization]") {
	frt::double_test("serializing into a vector", [] {
		const auto in_value1 = uint16_t{0x0A0B};
		const auto in_value2 = uint32_t{0x01020304};
		const auto in_value3 = 2.35;

		static constexpr auto value1_size = sizeof(in_value1);
		static constexpr auto value2_size = sizeof(in_value2);
		static constexpr auto value3_size = sizeof(in_value3);

		auto buf = std::vector<unsigned char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value1) == value1_size);
		FRT_CHECK(buf == std::vector<unsigned char>{0x0B, 0x0A});

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value2) == value2_size);
		FRT_CHECK(buf == std::vector<unsigned char>{0x0B, 0x0A, 0x04, 0x03, 0x02, 0x01});

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value3) == value3_size);

		auto out_value1 = uint16_t{};
		auto out_value2 = uint32_t{};
		auto out_value3 = double{};

		auto reader = fr::SpanReader{buf};

		auto res1 = fr::SbsDataFormat::decode(reader, out_value1);
		FRT_REQUIRE(res1);
		FRT_CHECK(*res1 == value1_size);
		FRT_CHECK(out_value1 == in_value1);

		auto res2 = fr::SbsDataFormat::decode(reader, out_value2);
		FRT_REQUIRE(res2);
		FRT_CHECK(*res2 == value2_size);
		FRT_CHECK(out_value2 == in_value2);

		auto res3 = fr::SbsDataFormat::decode(reader, out_value3);
		FRT_REQUIRE(res3);
		FRT_CHECK(*res3 == value3_size);
		FRT_CHECK(out_value3 == in_value3);
	});
	frt::double_test("serializing into an array which is too small", [] {
		auto in_value1 = uint16_t{0x0A0B};
		auto in_value2 = uint32_t{0x01020304};

		auto buf = std::array<unsigned char, 5>{};
		auto writer = fr::SpanWriter{buf};

		auto res1 = fr::SbsDataFormat::encode(writer, in_value1);
		FRT_REQUIRE(res1);
		FRT_CHECK(*res1 == 2);

		auto res2 = fr::SbsDataFormat::encode(writer, in_value2);
		FRT_CHECK(!res2);
		FRT_CHECK(res2.has_error<fr::BufferOverrun>());
	});
	frt::double_test("deserializing from an array which is too small", [] {
		auto out_value = uint64_t{};

		auto buf = std::array<unsigned char, 5>{};
		auto reader = fr::SpanReader{buf};

		auto res = fr::SbsDataFormat::decode(reader, out_value);
		FRT_CHECK(!res);
		FRT_CHECK(res.has_error<fr::BufferOverrun>());
	});
}

TEST_CASE("SbsDataFormat.custom", "[u][engine][core][serialization]") {
	test_common_serde_scenarios("using a friend function", [] static {
		return FriendCustomizedStruct{55, "abcdef"};
	});
	test_common_serde_scenarios("using a static member function", [] static {
		return StaticCustomizedStruct{55, "abcdef"};
	});
}

TEST_CASE("SbsDataFormat.described", "[u][engine][core][serialization]") {
	frt::double_test("OptOut mode serializes fields unless explicitly opted out", [] {
		// _x: explicit Serializable{true} -> included
		// _y: no attribute, defaults to included in OptOut mode -> included
		// _z: explicit Serializable{false} -> excluded
		const auto in_value = DescribedOptOutClass{11, 22, "abcdef"};
		const auto value_size = serialized_size(in_value);

		auto buf = std::vector<unsigned char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value) == value_size);

		auto out_value = DescribedOptOutClass{99, 99, "untouched"};
		auto reader = fr::SpanReader{buf};

		auto res = fr::SbsDataFormat::decode(reader, out_value);
		FRT_REQUIRE(res);
		FRT_CHECK(*res == value_size);
		FRT_CHECK(out_value == DescribedOptOutClass{11, 22, "untouched"});
		FRT_CHECK(out_value != in_value);
	});
	frt::double_test<false>("OptIn mode serializes fields only when explicitly opted in", [] {
		// _x: explicit Serializable{true} -> included
		// _y: no attribute, defaults to excluded in OptIn mode -> excluded
		// _z: explicit Serializable{true} -> included
		const auto in_value = DescribedOptInClass{11, 22, "abcdef"};
		const auto value_size = serialized_size(in_value);

		auto buf = std::vector<unsigned char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value) == value_size);

		auto out_value = DescribedOptInClass{99, 99, "untouched"};
		auto reader = fr::SpanReader{buf};

		auto res = fr::SbsDataFormat::decode(reader, out_value);
		FRT_REQUIRE(res);
		FRT_CHECK(*res == value_size);
		FRT_CHECK(out_value == DescribedOptInClass{11, 99, "abcdef"});
		FRT_CHECK(out_value != in_value);
	});
	frt::double_test<false>("class-level Serializable{true} attribute behaves like OptOut", [] {
		const auto in_value = DescribedSerializableClass{11, 22, "abcdef"};
		const auto value_size = serialized_size(in_value);

		auto buf = std::vector<unsigned char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value) == value_size);

		auto out_value = DescribedSerializableClass{99, 99, "untouched"};
		auto reader = fr::SpanReader{buf};

		auto res = fr::SbsDataFormat::decode(reader, out_value);
		FRT_REQUIRE(res);
		FRT_CHECK(*res == value_size);
		FRT_CHECK(out_value == DescribedSerializableClass{11, 22, "untouched"});
	});
	frt::double_test<false>("bases, fields and properties, respecting inclusion rules", [] {
		// BaseA: aggregate, serializable -> included (OptIn: opted in via serializability)
		// BaseB: described, class-level Serializable{true} -> included
		// BaseC: raw pointer field, unserializable -> excluded
		// _x: no attribute, defaults to excluded in OptIn mode -> excluded
		// y (property): explicit Serializable{true} -> included
		// _z: explicit Serializable{true} -> included
		const auto in_value = DescribedWithBasesAndProps{
			SimpleEnum::B, 5, "hello", 77, "world", 3.5
		};
		const auto value_size = serialized_size(in_value);

		auto buf = std::vector<unsigned char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value) == value_size);

		auto out_value = DescribedWithBasesAndProps{
			SimpleEnum::A, 0, "", 0, "", 0.0
		};
		auto reader = fr::SpanReader{buf};

		auto res = fr::SbsDataFormat::decode(reader, out_value);
		FRT_REQUIRE(res);
		FRT_CHECK(*res == value_size);
		FRT_CHECK(out_value == (DescribedWithBasesAndProps{
			SimpleEnum::B, 5, "hello", 0, "world", 3.5
		}));
		FRT_CHECK(out_value != in_value);
	});
	frt::double_test("serialializing into an array which is too small", [] {
		const auto get_value = [] { return DescribedOptOutClass{11, 22, "abcdef"}; };
		const auto in_value = get_value();
		static constexpr auto value_size = serialized_size(get_value());

		auto buf = std::array<std::byte, value_size - 2zu>{};
		auto writer = fr::SpanWriter{buf};

		auto res = fr::SbsDataFormat::encode(writer, in_value);
		FRT_CHECK(!res);
		FRT_CHECK(res.template has_error<fr::BufferOverrun>());
	});
	frt::double_test("deserializing from a span which is too small", [] {
		const auto in_value = DescribedOptInClass{11, 22, "abcdef"};

		auto buf = std::vector<char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

		auto out_value = DescribedOptInClass{};

		auto reader = fr::SpanReader{std::span<char>(buf.data(), buf.data() + 2zu)};
		auto res = fr::SbsDataFormat::decode(reader, out_value);
		FRT_CHECK(!res);
		FRT_CHECK(res.template has_error<fr::BufferOverrun>());
	});
}

TEST_CASE("SbsDataFormat.optionals", "[u][engine][core][serialization]") {
	frt::double_test("serializing into a vector", [] {
		const auto in_value1 = std::optional<int>{};
		const auto in_value2 = std::optional<int>{67};

		static constexpr auto value1_size = sizeof(bool);
		static constexpr auto value2_size = sizeof(bool) + sizeof(int);

		auto buf = std::vector<unsigned char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value1) == value1_size);
		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value2) == value2_size);

		auto out_value1 = std::optional<int>{};
		auto out_value2 = std::optional<int>{};
		auto out_value3 = std::optional<int>{5};
		auto out_value4 = std::optional<int>{5};

		auto reader1 = fr::SpanReader{buf};
		auto reader2 = fr::SpanReader{buf};

		auto res1 = fr::SbsDataFormat::decode(reader1, out_value1);
		FRT_REQUIRE(res1);
		FRT_CHECK(*res1 == value1_size);
		FRT_CHECK(out_value1 == in_value1);

		auto res2 = fr::SbsDataFormat::decode(reader1, out_value2);
		FRT_REQUIRE(res2);
		FRT_CHECK(*res2 == value2_size);
		FRT_CHECK(out_value2 == in_value2);

		auto res3 = fr::SbsDataFormat::decode(reader2, out_value3);
		FRT_REQUIRE(res3);
		FRT_CHECK(*res3 == value1_size);
		FRT_CHECK(out_value3 == in_value1);

		auto res4 = fr::SbsDataFormat::decode(reader2, out_value4);
		FRT_REQUIRE(res4);
		FRT_CHECK(*res4 == value2_size);
		FRT_CHECK(out_value4 == in_value2);
	});
	frt::double_test("serialializing into an array which is too small", [] {
		const auto in_value = std::optional<int>{5};
		static constexpr auto value_size = sizeof(bool) + sizeof(int);

		auto buf = std::array<std::byte, value_size - 3>{};
		auto writer = fr::SpanWriter{buf};

		auto res = fr::SbsDataFormat::encode(writer, in_value);
		FRT_CHECK(!res);
		FRT_CHECK(res.template has_error<fr::BufferOverrun>());
	});
	frt::double_test("deserializing from a span which is too small", [] {
		const auto in_value = std::optional<int>{66};
		static constexpr auto value_size = sizeof(bool) + sizeof(int);

		auto buf = std::vector<char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

		auto out_value = std::optional<int>{};

		auto reader = fr::SpanReader{std::span<char>(buf.data(), buf.data() + value_size - 2)};
		auto res = fr::SbsDataFormat::decode(reader, out_value);
		FRT_CHECK(!res);
		FRT_CHECK(res.template has_error<fr::BufferOverrun>());
	});
}

TEST_CASE("SbsDataFormat.strings", "[u][engine][core][serialization]") {
	constexpr auto test_strings = []<class C>(const auto& name, fr::MpType<C>) {
		test_common_serde_scenarios<false>(
			name,
			[] static { return std::basic_string<C>(frt::small_text_for<C>); },
			[] static { return std::basic_string<C>(frt::lorem_text_for<C>); },
			fr::size_c<sizeof(size_t) +  fr::str_length_bytes(frt::small_text_for<C>)>,
			fr::size_c<sizeof(size_t) + fr::str_length_bytes(frt::lorem_text_for<C>)>
		);
	};
	test_strings("std::string", fr::mp_type<char>);
	test_strings("std::wstring", fr::mp_type<wchar_t>);
	test_strings("std::u8string", fr::mp_type<char8_t>);
	test_strings("std::u16string", fr::mp_type<char16_t>);
	test_strings("std::u32string", fr::mp_type<char32_t>);
}

TEST_CASE("SbsDataFormat.arrays", "[u][engine][core][serialization]") {
	SECTION("serializing into a vector") {
		frt::double_test([] {
			const auto in_value1 = std::array<int, 3>{11, 22, 33};
			const auto in_value2 = std::array<int, 0>{};
			const int in_value3[3] = {11, 22, 33};

			static constexpr auto value1_size = sizeof(int) * std::size(in_value1);
			static constexpr auto value2_size = sizeof(int) * std::size(in_value2);
			static constexpr auto value3_size = sizeof(int) * std::size(in_value3);

			auto buf = std::vector<unsigned char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value1) == value1_size);
			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value2) == value2_size);
			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value3) == value3_size);

			auto out_value1 = std::array<int, 3>{};
			auto out_value2 = std::array<int, 0>{};
			int out_value3[3] = {};

			auto reader = fr::SpanReader{buf};

			auto res1 = fr::SbsDataFormat::decode(reader, out_value1);
			FRT_REQUIRE(res1);
			FRT_CHECK(*res1 == value1_size);
			FRT_CHECK(out_value1 == in_value1);

			auto res2 = fr::SbsDataFormat::decode(reader, out_value2);
			FRT_REQUIRE(res2);
			FRT_CHECK(*res2 == value2_size);
			FRT_CHECK(out_value2 == in_value2);

			auto res3 = fr::SbsDataFormat::decode(reader, out_value3);
			FRT_REQUIRE(res3);
			FRT_CHECK(*res3 == value3_size);
			FRT_CHECK(std::ranges::equal(out_value3, in_value3));
		});
	}
	SECTION("serialializing into an array which is too small") {
		frt::double_test([] {
			const auto in_value = std::array<int, 3>{11, 22, 33};
			static constexpr auto value_size = sizeof(int) * std::size(in_value);

			auto buf = std::array<std::byte, value_size - 3>{};
			auto writer = fr::SpanWriter{buf};

			auto res = fr::SbsDataFormat::encode(writer, in_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());
		});
	}
	SECTION("deserializing from a span which is too small") {
		frt::double_test([] {
			const auto in_value = std::array<int, 3>{11, 22, 33};
			static constexpr auto value_size = sizeof(int) * std::size(in_value);

			auto buf = std::vector<char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

			auto out_value = std::array<int, 3>{};

			auto reader = fr::SpanReader{std::span<char>(buf.data(), buf.data() + value_size - 3)};
			auto res = fr::SbsDataFormat::decode(reader, out_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());
		});
	}
}

TEST_CASE("SbsDataFormat.maps", "[u][engine][core][serialization]") {
	static constexpr auto empty_size = sizeof(size_t);
	{
		static constexpr auto make_values = [] static {
			return std::to_array<std::pair<const std::string, int>>(
				{{"abcdef", 22}, {frt::lorem_text, 44}, {{}, 66}, {"123", 88}}
			);
		};
		constexpr auto values_size = serialized_size(fr::as_span(make_values()));
		test_common_serde_scenarios<false>(
			"std::map",
			[] static { return std::map<std::string, int>(std::from_range, make_values()); },
			[] static { return std::map<std::string, int>{}; },
			fr::size_c<values_size>,
			fr::size_c<empty_size>
		);
		test_common_serde_scenarios<false>(
			"std::unordered_map",
			[] static {
				return std::unordered_map<std::string, int>(std::from_range, make_values());
			},
			[] static { return std::unordered_map<std::string, int>{}; },
			fr::size_c<values_size>,
			fr::size_c<empty_size>
		);
	}
	{
		static constexpr auto make_values = [] static {
			return std::to_array<std::pair<const std::string, int>>({
				{"abcdef", 22}, {frt::lorem_text, 44}, {{}, 66}, {"123", 88}, {frt::lorem_text, 45},
				{"123", 89}, {{}, 67}, {{}, 68}
			});
		};
		constexpr auto values_size = serialized_size(fr::as_span(make_values()));
		test_common_serde_scenarios<false>(
			"std::multimap",
			[] static { return std::multimap<std::string, int>(std::from_range, make_values()); },
			[] static { return std::multimap<std::string, int>{}; },
			fr::size_c<values_size>,
			fr::size_c<empty_size>
		);
		test_common_serde_scenarios<false>(
			"std::unordered_multimap",
			[] static {
				return std::unordered_multimap<std::string, int>(std::from_range, make_values());
			},
			[] static { return std::unordered_multimap<std::string, int>{}; },
			fr::size_c<values_size>,
			fr::size_c<empty_size>
		);
	}
}

TEST_CASE("SbsDataFormat.sets", "[u][engine][core][serialization]") {
	{
		static constexpr int values[] = {44, 22, 88, 66};
		static constexpr auto value_size = serialized_size(std::span<const int>(values));
		test_common_serde_scenarios<false>(
			"std::set",
			[] static { return std::set<int>(std::from_range, values); },
			fr::size_c<value_size>
		);
		test_common_serde_scenarios<false>(
			"std::unordered_set",
			[] static { return std::unordered_set<int>(std::from_range, values); },
			fr::size_c<value_size>
		);
	}
	{
		static constexpr int values[] = {22, 44, 22, 22, 66, 88, 44};
		static constexpr auto value_size = serialized_size(std::span<const int>(values));
		test_common_serde_scenarios<false>(
			"std::multiset",
			[] static { return std::multiset<int>(std::from_range, values); },
			fr::size_c<value_size>
		);
		test_common_serde_scenarios<false>(
			"std::unordered_multiset",
			[] static { return std::unordered_multiset<int>(std::from_range, values); },
			fr::size_c<value_size>
		);
	}
}

TEST_CASE("SbsDataFormat.vectors", "[u][engine][core][serialization]") {
	static constexpr int values[] = {22, 44, 66, 88};
	test_common_serde_scenarios(
		{},
		[] static { return std::vector<int>(std::from_range, values); },
		fr::size_c<sizeof(size_t) + sizeof(int) * std::size(values)>
	);
}

TEST_CASE("SbsDataFormat.variants", "[u][engine][core][serialization]") {
	// NOTE: Order is important for libstdc++. `int64_t` can't come first.
	// libstdc++ constructs scalar types in a temporary, so that exceptions thrown during its
	// construction don't affect the internal state
	using Var = std::variant<FriendCustomizedStruct, int64_t>;
	using Index = fr::SbsDataFormat::VariantIndexType<Var>;
	test_common_serde_scenarios(
		"non-valueless variants",
		[] static { return Var{std::in_place_type<int64_t>, 67}; },
		[] static { return Var{std::in_place_type<FriendCustomizedStruct>, 15, "abc"}; },
		fr::size_c<sizeof(Index) + sizeof(int64_t)>,
		fr::size_c<sizeof(Index) + sizeof(int) + sizeof(size_t) + 3>
	);
	// C++23 bans excepion throwing in constexpr context, run these checks only at runtime
	SECTION("serializing a valueless variant into a vector") {
		const auto in_value = fr::make_valueless_variant<Var>();
		FRT_REQUIRE(in_value.valueless_by_exception());

		static constexpr auto value_size = sizeof(Index);

		auto buf = std::vector<unsigned char>{};
		auto writer = fr::VectorWriter{buf};

		FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value) == value_size);

		auto out_value1 = Var{0};
		auto out_value2 = fr::make_valueless_variant<Var>();

		auto reader1 = fr::SpanReader{buf};
		auto reader2 = fr::SpanReader{buf};

		auto res1 = fr::SbsDataFormat::decode(reader1, out_value1);
		FRT_REQUIRE(res1);
		FRT_CHECK(*res1 == value_size);
		FRT_CHECK(out_value1 == in_value);

		auto res2 = fr::SbsDataFormat::decode(reader2, out_value2);
		FRT_REQUIRE(res2);
		FRT_CHECK(*res2 == value_size);
		FRT_CHECK(out_value2 == in_value);
	}
}

TEST_CASE("SbsDataFormat.records", "[u][engine][core][serialization]") {
	test_common_serde_scenarios("aggregate", [] static {
		return ComplexAggregate{
			{-23, "abcdef"}, {3.f, 99}, {5, 6, 7}
		};
	});
	constexpr auto make_tuple = [] static {
		return std::tuple<FriendCustomizedStruct, SimpleAggregate, std::array<int16_t, 3>>{
			{-23, "abcdef"}, {3.f, 99}, {5, 6, 7}
		};
	};
	test_common_serde_scenarios("tuple", make_tuple);
}
