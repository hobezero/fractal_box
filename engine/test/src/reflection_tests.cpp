#include "fractal_box/core/meta/description_types.hpp"
#include "fractal_box/core/meta/reflection.hpp"
#include "fractal_box/core/meta/type_name.hpp"

#include <optional>
#include <tuple>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/algorithm.hpp"

// Type name tests
// ===============

// NOTE: Can't use an anonymous namespace because `type_name` would produce implementation-
// specific strings which can't be asserted in unit tests

struct KeplerTestClassInGlobalNs {
	long long my_int;
};

template<class T, class U>
struct KeplerTestTemplateClass {
	long long my_int;
};

namespace frt {

template<class T>
struct S { };

struct ClassInANs {
	struct InnerClass {
		long long my_int;
	};
	long long my_int;
};

namespace inner {

struct ClassInAnInnerNs {
	long long my_int;
};

template<class T, auto V>
struct TemplateClassInAnInnerNs {
	long long my_int;
};

} // namespace inner
} // namespace frt

using namespace std::string_view_literals;
using namespace std::string_literals;

static constexpr std::string_view type_names[] = {
	fr::type_name<void>,
	fr::type_name<int>,
	fr::type_name<const int>,
	fr::type_name<float>,
	fr::type_name<int*>,
	fr::type_name<std::string>,
	fr::type_name<std::wstring>,
	fr::type_name<fr::ValueC<32>>,
	fr::type_name<fr::ValueC<33>>,
};

TEST_CASE("type_name", "[u][engine][core][reflection]") {
	// TODO: Test that we preserve information about member/function pointers, NTTP values, etc.
	STATIC_CHECK(fr::type_name_lit<void> == "void"sv);
	STATIC_CHECK("void"sv == fr::type_name_lit<void>);
	STATIC_CHECK(fr::type_name_lit<int> == "int"sv);
	STATIC_CHECK(fr::type_name_lit<char> == "char"sv);
	STATIC_CHECK(fr::type_name_lit<long double> == "long double"sv);

	STATIC_CHECK(fr::type_name_lit<KeplerTestClassInGlobalNs>
		== "KeplerTestClassInGlobalNs"sv);
	STATIC_CHECK(fr::type_name_lit<KeplerTestTemplateClass<int, long double>>
		== "KeplerTestTemplateClass<int, long double>"sv);
	STATIC_CHECK(fr::type_name_lit<frt::ClassInANs>
		== "frt::ClassInANs"sv);
	STATIC_CHECK(fr::type_name_lit<frt::ClassInANs::InnerClass>
		== "frt::ClassInANs::InnerClass"sv);
	STATIC_CHECK(fr::type_name_lit<frt::inner::ClassInAnInnerNs>
		== "frt::inner::ClassInAnInnerNs"sv);

	STATIC_CHECK(fr::type_name_lit<std::tuple<int, char*>>
		!= fr::type_name_lit<std::tuple<int, const char*>>);

	CHECK(fr::test_all_unique_small(type_names));
}

TEST_CASE("unqualified_type_name", "[u][engine][core][reflection]") {
	// TODO: Test that we preserve information about member/function pointers, NTTP values, etc.
	STATIC_CHECK(fr::unqualified_type_name_lit<void> == "void"sv);
	STATIC_CHECK("void"sv == fr::unqualified_type_name_lit<void>);
	STATIC_CHECK(fr::unqualified_type_name_lit<int> == "int"sv);
	STATIC_CHECK(fr::unqualified_type_name_lit<char> == "char"sv);
	STATIC_CHECK(fr::unqualified_type_name_lit<long double> == "long double"sv);

	STATIC_CHECK(fr::unqualified_type_name_lit<KeplerTestClassInGlobalNs>
		== "KeplerTestClassInGlobalNs"sv);
	STATIC_CHECK(fr::unqualified_type_name_lit<KeplerTestTemplateClass<int, long double>>
		== "KeplerTestTemplateClass<int, long double>"sv);
	STATIC_CHECK(fr::unqualified_type_name_lit<frt::ClassInANs>
		== "ClassInANs"sv);
	STATIC_CHECK(fr::unqualified_type_name_lit<frt::ClassInANs::InnerClass>
		== "InnerClass"sv);
	STATIC_CHECK(fr::unqualified_type_name_lit<frt::inner::ClassInAnInnerNs>
		== "ClassInAnInnerNs"sv);
	STATIC_CHECK(
		fr::unqualified_type_name_lit<
			frt::inner::TemplateClassInAnInnerNs<frt::S<int>, 5>
		>
		== "TemplateClassInAnInnerNs<frt::S<int>, 5>"sv
	);
}

TEST_CASE("clean_type_name", "[u][engine][core][reflection]") {
	// TODO: Test that we preserve information about member/function pointers, NTTP values, etc.
	STATIC_CHECK(fr::clean_type_name_lit<void> == "void"sv);
	STATIC_CHECK("void"sv == fr::clean_type_name_lit<void>);
	STATIC_CHECK(fr::clean_type_name_lit<int> == "int"sv);
	STATIC_CHECK(fr::clean_type_name_lit<char> == "char"sv);
	STATIC_CHECK(fr::clean_type_name_lit<long double> == "long double"sv);

	STATIC_CHECK(fr::clean_type_name_lit<KeplerTestClassInGlobalNs>
		== "KeplerTestClassInGlobalNs"sv);
	STATIC_CHECK(fr::clean_type_name_lit<KeplerTestTemplateClass<int, long double>>
		== "KeplerTestTemplateClass"sv);
	STATIC_CHECK(fr::clean_type_name_lit<frt::ClassInANs>
		== "ClassInANs"sv);
	STATIC_CHECK(fr::clean_type_name_lit<frt::ClassInANs::InnerClass>
		== "InnerClass"sv);
	STATIC_CHECK(fr::clean_type_name_lit<frt::inner::ClassInAnInnerNs>
		== "ClassInAnInnerNs"sv);
	STATIC_CHECK(fr::clean_type_name_lit<frt::inner::TemplateClassInAnInnerNs<int, 5>>
		== "TemplateClassInAnInnerNs"sv);
}

TEST_CASE("member_name", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::member_name<&KeplerTestClassInGlobalNs::my_int> == "my_int"sv);
	STATIC_CHECK(fr::member_name<&KeplerTestTemplateClass<int*, long double>::my_int>
		== "my_int"sv);
	STATIC_CHECK(fr::member_name<&frt::ClassInANs::my_int> == "my_int"sv);
	STATIC_CHECK(fr::member_name<&frt::ClassInANs::InnerClass::my_int> == "my_int"sv);
	STATIC_CHECK(fr::member_name<&frt::inner::ClassInAnInnerNs::my_int> == "my_int"sv);
	STATIC_CHECK(fr::member_name<&frt::inner::TemplateClassInAnInnerNs<int, 40>::my_int>
		== "my_int"sv);
}

template<fr::c_hash_digest Digest>
static constexpr Digest type_hashes[] = {
	fr::type_hash<Digest, void>,
	fr::type_hash<Digest, int>,
	fr::type_hash<Digest, const int>,
	fr::type_hash<Digest, float>,
	fr::type_hash<Digest, int*>,
	fr::type_hash<Digest, std::string>,
	fr::type_hash<Digest, std::wstring>,
	fr::type_hash<Digest, fr::ValueC<32>>,
	fr::type_hash<Digest, fr::ValueC<33>>,
};

TEST_CASE("type_hash", "[u][engine][core][reflection]") {
	// TODO: Test that we preserve information about member/function pointers, NTTP values, etc.
	CHECK(fr::test_all_unique_small(type_hashes<fr::HashDigest32>));
	CHECK(fr::test_all_unique_small(type_hashes<fr::HashDigest64>));
}

// Description-based reflection tests
// ==================================

namespace frt {

struct MySerializable {
	[[maybe_unused]] friend
	auto operator<=>(MySerializable, MySerializable) = default;

public:
	bool value = true;
};

struct MyHashable {
	[[maybe_unused]] friend
	auto operator<=>(MyHashable, MyHashable) = default;

public:
	bool value = true;
};

struct MyValueProp {
	[[maybe_unused]] friend
	auto operator<=>(MyValueProp, MyValueProp) = default;

public:
	int value;
};

struct MyDummyProp { };

template<class T>
struct MyTypeProp { };

struct MyParentA { };
struct MyParentB { };
struct MyParentC { };

struct MyGadget {
	[[maybe_unused]] friend consteval
	auto kepler_describe(MyGadget) {
		return fr::class_desc<>;
	}
};

struct MyWidget: public MyParentA, protected MyParentB, private MyParentC {
	MyWidget() = default;

	constexpr
	MyWidget(int the_x, std::string the_yapp, const char* the_z):
		x{the_x},
		yapp(std::move(the_yapp)),
		z{the_z}
	{ }

	[[maybe_unused]] friend consteval
	auto kepler_describe(MyWidget) noexcept {
		return fr::class_desc<
			fr::Name<"MyWidget">,
			fr::DisplayName<"My Widget">,
			fr::Bases<MyParentA, MyParentB>,
			fr::Attributes<MyValueProp{42}, MyTypeProp<char>{}>,
			fr::Field<
				&MyWidget::x,
				fr::Name<"superX">,
				fr::Attributes<MySerializable{}, MyHashable{false}>
			>,
			fr::Bases<MyParentC>,
			fr::Field<
				&MyWidget::yapp,
				fr::Attributes<MySerializable{}>
			>,
			fr::Field<
				&MyWidget::z,
				fr::DisplayName<"zed">
			>,
			fr::Property<
				"u",
				std::string_view,
				[](const MyWidget& self) noexcept -> std::string_view { return self.u; },
				[](MyWidget& self, auto&& value) { self.u = std::forward<decltype(u)>(value); },
				fr::DisplayName<"U">,
				fr::Attributes<MyHashable{}, MySerializable{}>
			>,
			fr::Property<
				"w",
				float,
				&MyWidget::get_w,
				&MyWidget::set_w
			>,
			fr::Attributes<MyHashable{false}>
		>;
	}

	auto get_w() const noexcept -> float { return this->w; }
	void set_w(float value) noexcept { this->w = value; }

public:
	int x = 0;
	std::string yapp;
	const char* z = nullptr;
	std::string u;
	float w = 0.f;
};

} // namespace frt

TEST_CASE("Description-concepts", "[u][engine][core][reflection]") {
	STATIC_CHECK(std::same_as<fr::IsDescriptionName<fr::Name<"Abc">>, fr::TrueC>);

	STATIC_CHECK(fr::c_has_describe<frt::MyWidget>);
	STATIC_CHECK_FALSE(fr::c_has_describe<frt::MyHashable>);
}

TEST_CASE("Class-description.empty", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::refl_custom_name<frt::MyGadget> == "frt::MyGadget");
	STATIC_CHECK(fr::refl_display_name<frt::MyGadget> == "MyGadget");

	STATIC_CHECK(std::same_as<
		fr::ReflAttributes<frt::MyGadget>,
		fr::MpValueList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflBases<frt::MyGadget>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflFields<frt::MyGadget>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflProperties<frt::MyGadget>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<frt::MyGadget>,
		fr::MpList<>
	>);
	STATIC_CHECK_FALSE(fr::refl_has_attribute<frt::MyGadget, frt::MyHashable>);
	STATIC_CHECK_FALSE(fr::refl_has_attribute<frt::MyGadget, frt::MyValueProp>);
}

TEST_CASE("Class-description.non-empty", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::refl_custom_name<frt::MyWidget> == "MyWidget");
	STATIC_CHECK(fr::refl_display_name<frt::MyWidget> == "My Widget");

	STATIC_CHECK(std::same_as<
		fr::ReflBases<frt::MyWidget>,
		fr::MpList<frt::MyParentA, frt::MyParentB, frt::MyParentC>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflFields<frt::MyWidget>,
		fr::MpList<
			fr::Field<
				&frt::MyWidget::x,
				fr::Name<"superX">,
				fr::Attributes<frt::MySerializable{}, frt::MyHashable{false}>
			>,
			fr::Field<
				&frt::MyWidget::yapp,
				fr::Attributes<frt::MySerializable{}>
			>,
			fr::Field<
				&frt::MyWidget::z,
				fr::DisplayName<"zed">
			>
		>
	>);
	STATIC_CHECK(fr::mp_size<fr::ReflProperties<frt::MyWidget>> == 2zu);

	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<frt::MyWidget>,
		fr::MpList<int, std::string, const char*>
	>);

	STATIC_CHECK(std::same_as<
		fr::ReflAttributes<frt::MyWidget>,
		fr::MpValueList<frt::MyValueProp{42}, frt::MyTypeProp<char>{}, frt::MyHashable{false}>
	>);

	STATIC_CHECK(fr::refl_has_attribute<frt::MyWidget, frt::MyValueProp>);
	STATIC_CHECK_FALSE(fr::refl_has_attribute<frt::MyWidget, frt::MyDummyProp>);

	STATIC_CHECK(fr::refl_attribute<frt::MyWidget, frt::MyValueProp> == frt::MyValueProp{42});
	STATIC_CHECK(fr::refl_attribute<frt::MyWidget, frt::MyHashable> == frt::MyHashable{false});
}

TEST_CASE("Class-description.fields", "[u][engine][core][reflection]") {
	using Fields = fr::ReflFields<frt::MyWidget>;
	using X = fr::MpAt<Fields, 0>;
	using Y = fr::MpAt<Fields, 1>;
	using Z = fr::MpAt<Fields, 2>;

	STATIC_CHECK(fr::refl_custom_name<X> == "superX");
	STATIC_CHECK(fr::refl_custom_name<Y> == "yapp");
	STATIC_CHECK(fr::refl_custom_name<Z> == "z");

	STATIC_CHECK(fr::refl_display_name<X> == "superX");
	STATIC_CHECK(fr::refl_display_name<Y> == "yapp");
	STATIC_CHECK(fr::refl_display_name<Z> == "zed");

	STATIC_CHECK(fr::refl_field_ptr<X> == &frt::MyWidget::x);
	STATIC_CHECK(fr::refl_field_ptr<Y> == &frt::MyWidget::yapp);
	STATIC_CHECK(fr::refl_field_ptr<Z> == &frt::MyWidget::z);

	STATIC_CHECK(std::same_as<fr::ReflFieldPtrType<X>, decltype(&frt::MyWidget::x)>);
	STATIC_CHECK(std::same_as<fr::ReflFieldPtrType<Y>, decltype(&frt::MyWidget::yapp)>);
	STATIC_CHECK(std::same_as<fr::ReflFieldPtrType<Z>, decltype(&frt::MyWidget::z)>);

	STATIC_CHECK(std::same_as<fr::ReflFieldClassType<X>, frt::MyWidget>);
	STATIC_CHECK(std::same_as<fr::ReflFieldClassType<Y>, frt::MyWidget>);
	STATIC_CHECK(std::same_as<fr::ReflFieldClassType<Z>, frt::MyWidget>);

	STATIC_CHECK(std::same_as<fr::ReflFieldType<X>, int>);
	STATIC_CHECK(std::same_as<fr::ReflFieldType<Y>, std::string>);
	STATIC_CHECK(std::same_as<fr::ReflFieldType<Z>, const char*>);

	STATIC_CHECK(std::same_as<
		fr::ReflAttributes<X>,
		fr::MpValueList<frt::MySerializable{}, frt::MyHashable{false}>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflAttributes<Y>,
		fr::MpValueList<frt::MySerializable{}>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflAttributes<Z>,
		fr::MpValueList<>
	>);
}

TEST_CASE("Class-description.properties", "[u][engine][core][reflection]") {
	using Properties = fr::ReflProperties<frt::MyWidget>;
	using U = fr::MpAt<Properties, 0>;
	using W = fr::MpAt<Properties, 1>;

	STATIC_CHECK(fr::refl_custom_name<U> == "u");
	STATIC_CHECK(fr::refl_custom_name<W> == "w");

	STATIC_CHECK(fr::refl_display_name<U> == "U");
	STATIC_CHECK(fr::refl_display_name<W> == "w");

	STATIC_CHECK(std::same_as<
		fr::ReflPropertyType<U>,
		std::string_view
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflPropertyType<W>,
		float
	>);

	STATIC_CHECK(std::same_as<
		fr::ReflAttributes<U>,
		fr::MpValueList<frt::MyHashable{true}, frt::MySerializable{true}>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflAttributes<W>,
		fr::MpValueList<>
	>);

	auto widget = frt::MyWidget{};

	SECTION("get/set_property") {
		CHECK(fr::get_property<U>(widget) == std::string());
		fr::set_property<U>(widget, "abcdef");
		CHECK(fr::get_property<U>(widget) == "abcdef");

		CHECK(fr::get_property<W>(widget) == 0.f);
		fr::set_property<W>(widget, 1.25f);
		CHECK(fr::get_property<W>(widget) == 1.25f);
	}
}

TEST_CASE("Class-description.fields-and-properties", "[u][engine][core][reflection]") {
	using FPs = fr::ReflFieldsAndProperties<frt::MyWidget>;
	using X = fr::MpAt<FPs, 0>;
	using Y = fr::MpAt<FPs, 1>;
	using Z = fr::MpAt<FPs, 2>;
	using U = fr::MpAt<FPs, 3>;
	using W = fr::MpAt<FPs, 4>;

	auto widget = frt::MyWidget{};

	CHECK(fr::get_field_or_property<X>(widget) == 0);
	fr::set_field_or_property<X>(widget, 15);
	CHECK(fr::get_field_or_property<X>(widget) == 15);

	CHECK(fr::get_field_or_property<Y>(widget) == std::string());
	fr::set_field_or_property<Y>(widget, "ZXC");
	CHECK(fr::get_field_or_property<Y>(widget) == "ZXC");

	static constexpr const char* z_val = "12345";
	CHECK(fr::get_field_or_property<Z>(widget) == nullptr);
	fr::set_field_or_property<Z>(widget, z_val);
	CHECK(fr::get_field_or_property<Z>(widget) == z_val);

	CHECK(fr::get_field_or_property<U>(widget) == std::string());
	fr::set_field_or_property<U>(widget, "abcdef");
	CHECK(fr::get_field_or_property<U>(widget) == "abcdef");

	CHECK(fr::get_field_or_property<W>(widget) == 0.f);
	fr::set_property<W>(widget, 1.25f);
	CHECK(fr::get_property<W>(widget) == 1.25f);
}

TEST_CASE("Class-description.nth_field_name", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::nth_field_name<frt::MyWidget, 0> == "superX");
	STATIC_CHECK(fr::nth_field_name<frt::MyWidget, 1> == "yapp");
	STATIC_CHECK(fr::nth_field_name<frt::MyWidget, 2> == "z");
}

TEST_CASE("Class-description.apply_field", "[u][engine][core][reflection]") {
	using Fields = fr::ReflFields<frt::MyWidget>;
	using X = fr::MpAt<Fields, 0>;
	using Y = fr::MpAt<Fields, 1>;

	auto widget = frt::MyWidget{};
	widget.*X::ptr = 15;
	CHECK(widget.x == 15);
	fr::apply_field<X>(widget) = 52;
	CHECK(widget.x == 52);

	fr::apply_field<Y>(widget) = "abc";
	CHECK(fr::apply_field<Y>(widget) == "abc");
	CHECK(fr::apply_field<Y>(std::as_const(widget)) == "abc");
}

TEST_CASE("Class-description.get", "[u][engine][core][reflection]") {
	auto widget = frt::MyWidget{};

	fr::get<0>(widget) = 16;
	fr::get<1>(widget) = "abc";
	fr::get<2>(widget) = "qwert";

	CHECK(widget.x == 16);
	CHECK(widget.yapp == "abc");
	CHECK(std::string_view(widget.z) == "qwert"sv);

	CHECK(fr::get<0>(widget) == 16);
	CHECK(fr::get<1>(widget) == "abc");
	CHECK(std::string_view(fr::get<2>(widget)) == "qwert"sv);
}

TEST_CASE("Class-description.for_each_field", "[u][engine][core][reflection]") {
	static constexpr auto x_value = 15;
	static constexpr auto y_value = std::string("abc");
	static constexpr const char* z_value = "qwert";

	auto widget = frt::MyWidget{x_value, y_value, z_value};

	CHECK(widget.x == x_value);
	CHECK(widget.yapp == y_value);
	CHECK(widget.z == z_value);

	fr::for_each_field(widget, [](auto& member) {
		member = {};
	});

	CHECK(widget.x == 0);
	CHECK(widget.yapp == std::string{});
	CHECK(widget.z == nullptr);
}

TEST_CASE("Class-description.visit", "[u][engine][core][reflection]") {
	static constexpr auto x_value = 15;
	static constexpr auto y_value = std::string("abc");
	static constexpr const char* z_value = "qwert";

	auto widget = frt::MyWidget{x_value, y_value, z_value};

	CHECK(widget.x == x_value);
	CHECK(widget.yapp == y_value);
	CHECK(widget.z == z_value);

	CHECK(fr::visit(widget, [](auto&&... fields) {
		(..., (fields = {}));
		return sizeof...(fields);
	}) == 3);

	CHECK(widget.x == 0);
	CHECK(widget.yapp == std::string{});
	CHECK(widget.z == nullptr);
}

// Aggregate reflection
// ====================

namespace frt {

struct Fidget {
	const char* text;
	std::string blah_blah;
	std::optional<int> foo;
	std::tuple<float, float> bar;
};

struct B0 { };

struct B00 { };

struct B1 {
	int m0;
};

struct B2 {
	int m0;
	char m1;
};

struct B6 {
	int m0;
	B2 m1;
	std::tuple<int, int> m2;
	std::string m3;
	std::vector<int> m4;
	std::optional<int> m5;
};

struct B7 {
	int m0;
	B2 m1;
	std::tuple<int, int> m2;
	unsigned m3: 1;
	unsigned m4: 1;
	unsigned m5: 2;
	unsigned m6: 1;
};

struct D0WithEmptyBase: B0 { };

struct D4WithTwoEmptyBases: B0, B00 {
	float m0;
	D0WithEmptyBase m1;
	std::pair<float, char> m2;
	std::optional<int> m3;
};

struct D4WithBaseMember: B0, B00 {
	float m0;
	char m1;
	std::optional<int> m2;
	B00 m3;
};

struct D0WithNonEmptyBase: B1 { };

struct D1WithNonEmptyBase: B1 {
	float m0;
};

struct D4WithThreeBases: B1, B2, B6 {
	float m0;
	char m1;
	D1WithNonEmptyBase m2;
	std::pair<float, char> m3;
};

struct WithConstMembers {
	const int i;
	const void* const p;
};

struct WithArrayMembers {
	float x;
	int a[3];
};

} // namespace frt

TEST_CASE("Aggregate-reflection.refl_custom_name", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::refl_custom_name<frt::Fidget> == "frt::Fidget");
}

TEST_CASE("Aggregate-reflection.refl_display_name", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::refl_display_name<frt::Fidget> == "Fidget");
}

TEST_CASE("Aggregate-reflection.class-API", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::mp_size<fr::ReflBases<frt::Fidget>> == 0);

	STATIC_CHECK(fr::mp_size<fr::ReflFields<frt::Fidget>> == 4);

	STATIC_CHECK(fr::mp_size<fr::ReflProperties<frt::Fidget>> == 0);

	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<frt::Fidget>,
		fr::MpList<const char*, std::string, std::optional<int>, std::tuple<float, float>>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<frt::WithConstMembers>,
		fr::MpList<const int, const void* const>
	>);
#if 0 // NOTE: Causes hard compilation error. Seems unfixable in C++23
	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<frt::WithArrayMembers>,
		fr::MpList<int[4]>
	>);
#endif
	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<std::tuple<const int, const void* const, float>>,
		fr::MpList<const int, const void* const, float>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<std::pair<int, const char>>,
		fr::MpList<int, const char>
	>);
	STATIC_CHECK(std::same_as<
		fr::ReflDecomposition<std::array<int, 3>>,
		fr::MpList<int, int, int>
	>);

	STATIC_CHECK(fr::mp_size<fr::ReflAttributes<frt::Fidget>> == 0);
	STATIC_CHECK_FALSE(fr::refl_has_attribute<frt::Fidget, frt::MyHashable>);
	STATIC_CHECK(std::same_as<fr::ReflHasAttribute<frt::Fidget, frt::MyHashable>, fr::FalseC>);
}

TEST_CASE("Aggregate-reflection.fields", "[u][engine][core][reflection]") {
	using Fields = fr::ReflFields<frt::Fidget>;
	using F0 = fr::MpAt<Fields, 0>;
	using F1 = fr::MpAt<Fields, 1>;
	using F2 = fr::MpAt<Fields, 2>;
	using F3 = fr::MpAt<Fields, 3>;

	STATIC_CHECK(fr::mp_size<Fields> == 4);

	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 0> == "text");
	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 1> == "blah_blah");
	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 2> == "foo");
	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 3> == "bar");

	STATIC_CHECK(fr::refl_custom_name<F0> == "text");
	STATIC_CHECK(fr::refl_custom_name<F1> == "blah_blah");
	STATIC_CHECK(fr::refl_custom_name<F2> == "foo");
	STATIC_CHECK(fr::refl_custom_name<F3> == "bar");

	STATIC_CHECK(fr::refl_display_name<F0> == "text");
	STATIC_CHECK(fr::refl_display_name<F1> == "blah_blah");
	STATIC_CHECK(fr::refl_display_name<F2> == "foo");
	STATIC_CHECK(fr::refl_display_name<F3> == "bar");

	STATIC_CHECK(std::same_as<fr::ReflFieldClassType<F0>, frt::Fidget>);
	STATIC_CHECK(std::same_as<fr::ReflFieldClassType<F1>, frt::Fidget>);
	STATIC_CHECK(std::same_as<fr::ReflFieldClassType<F2>, frt::Fidget>);
	STATIC_CHECK(std::same_as<fr::ReflFieldClassType<F3>, frt::Fidget>);

	STATIC_CHECK(std::same_as<fr::ReflFieldType<F0>, const char*>);
	STATIC_CHECK(std::same_as<fr::ReflFieldType<F1>, std::string>);
	STATIC_CHECK(std::same_as<fr::ReflFieldType<F2>, std::optional<int>>);
	STATIC_CHECK(std::same_as<fr::ReflFieldType<F3>, std::tuple<float, float>>);

	STATIC_CHECK(std::same_as<fr::ReflAttributes<F0>, fr::MpValueList<>>);
	STATIC_CHECK(std::same_as<fr::ReflAttributes<F1>, fr::MpValueList<>>);
	STATIC_CHECK(std::same_as<fr::ReflAttributes<F2>, fr::MpValueList<>>);
	STATIC_CHECK(std::same_as<fr::ReflAttributes<F3>, fr::MpValueList<>>);

	STATIC_CHECK_FALSE(fr::refl_has_attribute<F0, frt::MyHashable>);
	STATIC_CHECK_FALSE(fr::refl_has_attribute<F1, frt::MyHashable>);
	STATIC_CHECK_FALSE(fr::refl_has_attribute<F2, frt::MyHashable>);
	STATIC_CHECK_FALSE(fr::refl_has_attribute<F3, frt::MyHashable>);
}

TEST_CASE("Aggregate-reflection.num_record_fields", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::num_record_fields<frt::Fidget> == 4);

	STATIC_CHECK(fr::num_record_fields<frt::B0> == 0);
	STATIC_CHECK(fr::num_record_fields<frt::B1> == 1);
	STATIC_CHECK(fr::num_record_fields<frt::B2> == 2);
	STATIC_CHECK(fr::num_record_fields<frt::B6> == 6);
	STATIC_CHECK(fr::num_record_fields<frt::B7> == 7);

	STATIC_CHECK(fr::num_record_fields<frt::D0WithEmptyBase> == 0);
	STATIC_CHECK(fr::num_record_fields<frt::D4WithTwoEmptyBases> == 4);
	STATIC_CHECK(fr::num_record_fields<frt::D4WithBaseMember> == 4);

	STATIC_CHECK(fr::num_record_fields<frt::D0WithNonEmptyBase> == 1);
	STATIC_CHECK(fr::num_record_fields<frt::D1WithNonEmptyBase> == 2);
	STATIC_CHECK(fr::num_record_fields<frt::D4WithThreeBases> == 7);

	STATIC_CHECK(fr::num_record_fields<std::tuple<int, char, float>> == 3);
	STATIC_CHECK(fr::num_record_fields<std::pair<int, int>> == 2);
	STATIC_CHECK(fr::num_record_fields<std::array<int, 5>> == 5);
}

TEST_CASE("Aggregate-reflection.num_fields", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::num_fields<frt::Fidget> == 4);

	STATIC_CHECK(fr::num_fields<frt::B0> == 0);
	STATIC_CHECK(fr::num_fields<frt::B1> == 1);
	STATIC_CHECK(fr::num_fields<frt::B2> == 2);
	STATIC_CHECK(fr::num_fields<frt::B6> == 6);
}

TEST_CASE("Aggregate-reflection.nth_field_name", "[u][engine][core][reflection]") {
	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 0> == "text");
	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 1> == "blah_blah");
	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 2> == "foo");
	STATIC_CHECK(fr::nth_field_name<frt::Fidget, 3> == "bar");

	STATIC_CHECK(fr::nth_field_name<frt::B6, 0> == "m0");
	STATIC_CHECK(fr::nth_field_name<frt::B6, 1> == "m1");
	STATIC_CHECK(fr::nth_field_name<frt::B6, 2> == "m2");
	STATIC_CHECK(fr::nth_field_name<frt::B6, 3> == "m3");
	STATIC_CHECK(fr::nth_field_name<frt::B6, 4> == "m4");
	STATIC_CHECK(fr::nth_field_name<frt::B6, 5> == "m5");
}

TEST_CASE("Aggregate-reflection.apply_field", "[u][engine][core][reflection]") {
	using Fields = fr::ReflFields<frt::Fidget>;
	using F0 = fr::MpAt<Fields, 0>;
	using F1 = fr::MpAt<Fields, 1>;
	using F2 = fr::MpAt<Fields, 2>;
	using F3 = fr::MpAt<Fields, 3>;

	static constexpr char val0[] = "abcdef";
	static constexpr auto val1 = "ZXCVB"s;
	static constexpr auto val2 = std::make_optional(15);
	static constexpr auto val3 = std::make_tuple(3.f, 2.1f);

	auto fidget = frt::Fidget{};

	fr::apply_field<F0>(fidget) = val0;
	fr::apply_field<F1>(fidget) = val1;
	fr::apply_field<F2>(fidget) = val2;
	fr::apply_field<F3>(fidget) = val3;

	CHECK(fidget.text == val0);
	CHECK(fidget.blah_blah == val1);
	CHECK(fidget.foo == val2);
	CHECK(fidget.bar == val3);

	CHECK(fr::apply_field<F0>(fidget) == val0);
	CHECK(fr::apply_field<F1>(fidget) == val1);
	CHECK(fr::apply_field<F2>(fidget) == val2);
	CHECK(fr::apply_field<F3>(fidget) == val3);
}

TEST_CASE("Aggregate-reflection.get", "[u][engine][core][reflection]") {
	auto b = frt::B2{14, 'm'};

	fr::get<0>(b) = 50;

	CHECK(fr::get<0>(b) == 50);
	CHECK(fr::get<1>(b) == 'm');
}

TEST_CASE("Aggregate-reflection.for_each_field", "[u][engine][core][reflection]") {
	auto fidget = frt::Fidget{
		.text = "abc",
		.blah_blah = "def",
		.foo = 80,
		.bar = {3.f, 2.1f}
	};

	fr::for_each_field(fidget, [](auto& member) {
		member = {};
	});
	CHECK(fidget.text == nullptr);
	CHECK(fidget.blah_blah == std::string());
	CHECK(fidget.foo == std::nullopt);
	CHECK(fidget.bar == std::make_tuple(0.f, 0.f));
}

TEST_CASE("Aggregate-reflection.visit", "[u][engine][core][reflection]") {
	const auto on_b2 = [](int& m0, char& m1) {
		m0 = 14;
		m1 = 'm';
		return 2;
	};

	auto b = frt::B2{};
	CHECK(fr::visit(b, on_b2) == 2);
	CHECK(b.m0 == 14);
	CHECK(b.m1 == 'm');
}
