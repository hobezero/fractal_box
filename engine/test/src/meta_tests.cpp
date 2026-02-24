#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"

#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/string_literal.hpp"

namespace {

struct A { };
union B { };
struct C { };
struct D { };

template<class...>
struct MyTemplate { };

} // namespace

// concepts.hpp tests
// ------------------

TEST_CASE("cv_or_ref", "[u][engine][core][meta]") {
	STATIC_CHECK_FALSE(fr::c_cv_or_ref<int>);

	STATIC_CHECK_FALSE(fr::c_cv_or_ref<int*>);
	STATIC_CHECK_FALSE(fr::c_cv_or_ref<const int*>);
	STATIC_CHECK_FALSE(fr::c_cv_or_ref<volatile int*>);
	STATIC_CHECK_FALSE(fr::c_cv_or_ref<const volatile int*>);

	STATIC_CHECK(fr::c_cv_or_ref<const int>);
	STATIC_CHECK(fr::c_cv_or_ref<volatile int>);
	STATIC_CHECK(fr::c_cv_or_ref<const volatile int>);

	STATIC_CHECK(fr::c_cv_or_ref<int&>);
	STATIC_CHECK(fr::c_cv_or_ref<const int&>);
	STATIC_CHECK(fr::c_cv_or_ref<volatile int&>);
	STATIC_CHECK(fr::c_cv_or_ref<const volatile int&>);

	STATIC_CHECK(fr::c_cv_or_ref<int&&>);
	STATIC_CHECK(fr::c_cv_or_ref<const int&&>);
	STATIC_CHECK(fr::c_cv_or_ref<volatile int&&>);
	STATIC_CHECK(fr::c_cv_or_ref<const volatile int&&>);

	STATIC_CHECK(fr::c_cv_or_ref<const int* const>);
	STATIC_CHECK(fr::c_cv_or_ref<volatile int* const>);
	STATIC_CHECK(fr::c_cv_or_ref<const volatile int* const>);

	STATIC_CHECK(fr::c_cv_or_ref<int*&>);
	STATIC_CHECK(fr::c_cv_or_ref<const int*&>);
	STATIC_CHECK(fr::c_cv_or_ref<volatile int*&>);
	STATIC_CHECK(fr::c_cv_or_ref<const volatile int*&>);

	STATIC_CHECK(fr::c_cv_or_ref<int*&&>);
	STATIC_CHECK(fr::c_cv_or_ref<const int*&&>);
	STATIC_CHECK(fr::c_cv_or_ref<volatile int*&&>);
	STATIC_CHECK(fr::c_cv_or_ref<const volatile int*&&>);
}

TEST_CASE("c_pure_object", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::c_pure_object<int>);
	STATIC_CHECK(fr::c_pure_object<A>);

	STATIC_CHECK_FALSE(fr::c_pure_object<void>);

	STATIC_CHECK_FALSE(fr::c_pure_object<const int*>);
	STATIC_CHECK_FALSE(fr::c_pure_object<volatile int*>);
	STATIC_CHECK_FALSE(fr::c_pure_object<const volatile int*>);

	STATIC_CHECK_FALSE(fr::c_pure_object<const int>);
	STATIC_CHECK_FALSE(fr::c_pure_object<volatile int>);
	STATIC_CHECK_FALSE(fr::c_pure_object<const volatile int>);

	STATIC_CHECK_FALSE(fr::c_pure_object<int&>);
	STATIC_CHECK_FALSE(fr::c_pure_object<const int&>);
	STATIC_CHECK_FALSE(fr::c_pure_object<volatile int&>);
	STATIC_CHECK_FALSE(fr::c_pure_object<const volatile int&>);

	STATIC_CHECK_FALSE(fr::c_pure_object<int&&>);
	STATIC_CHECK_FALSE(fr::c_pure_object<const int&&>);
	STATIC_CHECK_FALSE(fr::c_pure_object<volatile int&&>);
	STATIC_CHECK_FALSE(fr::c_pure_object<const volatile int&&>);

	STATIC_CHECK_FALSE(fr::c_pure_object<int[]>);
	STATIC_CHECK_FALSE(fr::c_pure_object<int[5]>);
	STATIC_CHECK_FALSE(fr::c_pure_object<void (int, char)>);
}

TEST_CASE("CopyConst", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<fr::CopyConst<long, char>, long>);
	STATIC_CHECK(std::same_as<fr::CopyConst<long, const char>, const long>);
	STATIC_CHECK(std::same_as<fr::CopyConst<long, const volatile char>, const long>);

	STATIC_CHECK(std::same_as<fr::CopyConst<long, char&>, long>);
	STATIC_CHECK(std::same_as<fr::CopyConst<long, const char&>, const long>);
	STATIC_CHECK(std::same_as<fr::CopyConst<long, const volatile char&>, const long>);

	STATIC_CHECK(std::same_as<fr::CopyConst<long, char&&>, long>);
	STATIC_CHECK(std::same_as<fr::CopyConst<long, const char&&>, const long>);
	STATIC_CHECK(std::same_as<fr::CopyConst<long, const volatile char&&>, const long>);
}

TEST_CASE("CopyCv", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<fr::CopyCv<long, char>, long>);
	STATIC_CHECK(std::same_as<fr::CopyCv<long, const char>, const long>);
	STATIC_CHECK(std::same_as<fr::CopyCv<long, const volatile char>, const volatile long>);

	STATIC_CHECK(std::same_as<fr::CopyCv<long, char&>, long>);
	STATIC_CHECK(std::same_as<fr::CopyCv<long, const char&>, const long>);
	STATIC_CHECK(std::same_as<fr::CopyCv<long, const volatile char&>, const volatile long>);

	STATIC_CHECK(std::same_as<fr::CopyCv<long, char&&>, long>);
	STATIC_CHECK(std::same_as<fr::CopyCv<long, const char&&>, const long>);
	STATIC_CHECK(std::same_as<fr::CopyCv<long, const volatile char&&>, const volatile long>);
}

TEST_CASE("CopyCvRef", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, char>, long>);
	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, const char>, const long>);
	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, const volatile char>, const volatile long>);

	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, char&>, long&>);
	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, const char&>, const long&>);
	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, const volatile char&>, const volatile long&>);

	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, char&&>, long&&>);
	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, const char&&>, const long&&>);
	STATIC_CHECK(std::same_as<fr::CopyCvRef<long, const volatile char&&>, const volatile long&&>);
}

// meta_basics.hpp tests
// ---------------------

TEST_CASE("MpIf", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<fr::MpLazyIf<true>::Type<int, char>, int>);
	STATIC_CHECK(std::same_as<fr::MpLazyIf<false>::Type<int, char>, char>);
}

TEST_CASE("c_mp_list", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::c_type_list<fr::MpList<>>);
	STATIC_CHECK(fr::c_type_list<fr::MpList<int>>);
	STATIC_CHECK(fr::c_type_list<fr::MpList<int, char>>);

	STATIC_CHECK(fr::c_type_list<std::tuple<int>>);
	STATIC_CHECK(fr::c_type_list<std::tuple<int, char>>);

	STATIC_CHECK_FALSE(fr::c_type_list<int>);
	STATIC_CHECK_FALSE(fr::c_type_list<std::integral_constant<int, 4>>);
}

TEST_CASE("c_specialization", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::c_specialization<MyTemplate<>, MyTemplate>);
	STATIC_CHECK(fr::c_specialization<MyTemplate<int>, MyTemplate>);
	STATIC_CHECK(fr::c_specialization<MyTemplate<int, char>, MyTemplate>);
	STATIC_CHECK(fr::c_specialization<std::string, std::basic_string>);

	STATIC_CHECK_FALSE(fr::c_specialization<MyTemplate<int, char>, fr::MpList>);
	STATIC_CHECK_FALSE(fr::c_specialization<std::string, std::basic_string_view>);
}

namespace {

struct Complete { };
struct Incomplete;

} // namespace

TEST_CASE("is_complete.1", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::is_complete<Complete>);
	STATIC_CHECK_FALSE(fr::is_complete<Incomplete>);
}

namespace {

struct Incomplete { };

} // namespace

TEST_CASE("is_complete.2", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::is_complete<Incomplete>);
}

// meta.hpp tests
// --------------

using List = fr::MpList<int, A, double, B, int, int, B>;
using LongList = fr::MpList<
	int, A, double, B, char, char*, int*, const int,
	std::string, void, long, const int, unsigned, short, unsigned short, std::string_view,
	void(int), int(void), int (*)(int), const char, volatile bool, const B&
>;

TEST_CASE("MpRename", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpRename<std::variant<int, A, B>, std::tuple>,
		std::tuple<int, A, B>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRename<std::variant<int, A, B>, fr::MpList>,
		fr::MpList<int, A, B>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRename<std::variant<int, A, B>, std::variant>,
		std::variant<int, A, B>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRename<fr::MpList<>, std::tuple>,
		std::tuple<>
	>);
}

TEST_CASE("ToMpList", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<fr::ToMpList<std::variant<int, A, double, B>>,
		fr::MpList<int, A, double, B>>);
	STATIC_CHECK(std::same_as<fr::ToMpList<std::tuple<B>>, fr::MpList<B>>);
	STATIC_CHECK(std::same_as<fr::ToMpList<fr::MpList<char, A>>, fr::MpList<char, A>>);
	STATIC_CHECK(std::same_as<fr::ToMpList<fr::MpList<>>, fr::MpList<>>);
}

template<class T>
using IsClass = typename std::is_class<T>::type;

TEST_CASE("c_mp_list_of", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::c_mp_list_of<fr::MpList<>, IsClass>);
	STATIC_CHECK(fr::c_mp_list_of<fr::MpList<std::string>, IsClass>);
	STATIC_CHECK(fr::c_mp_list_of<fr::MpList<std::string, Complete>, IsClass>);
	STATIC_CHECK(fr::c_mp_list_of<fr::MpList<A, C, std::string, Incomplete, Complete>, IsClass>);

	STATIC_CHECK_FALSE(fr::c_mp_list_of<fr::MpList<std::string, int, Complete>, IsClass>);
	STATIC_CHECK_FALSE(fr::c_mp_list_of<MyTemplate<>, IsClass>);
	STATIC_CHECK_FALSE(fr::c_mp_list_of<std::variant<std::string, Complete>, IsClass>);
}

TEST_CASE("mp_is_empty", "[u][engine][core][meta]") {
	STATIC_CHECK_FALSE(fr::mp_is_empty<std::tuple<int, char, double>>);
	STATIC_CHECK(fr::mp_is_empty<fr::MpList<>>);
}

TEST_CASE("MpSize", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_size<std::tuple<int, char, double>> == 3);
	STATIC_CHECK(fr::mp_size<fr::MpList<>> == 0);
}

TEST_CASE("MpAt", "[u][engine][core][meta]") {
	SECTION("list of a single type") {
		STATIC_CHECK(std::same_as<fr::MpAt<fr::MpList<short>, 0>, short>);
		STATIC_CHECK(std::same_as<fr::MpAt<fr::MpList<short>, 0>, short>);
	}
	SECTION("list of few types") {
		STATIC_CHECK(std::same_as<fr::MpAt<List, 0>, int>);
		STATIC_CHECK(std::same_as<fr::MpAt<List, 1>, A>);
		STATIC_CHECK(std::same_as<fr::MpAt<List, 2>, double>);
		STATIC_CHECK(std::same_as<fr::MpAt<List, 3>, B>);
	}
	SECTION("list of many types") {
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 3>, B>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 6>, int*>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 7>, const int>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 8>, std::string>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 9>, void>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 10>, long>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 12>, unsigned>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 13>, short>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 14>, unsigned short>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 15>, std::string_view>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 16>, void(int)>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 17>, int(void)>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 20>, volatile bool>);
		STATIC_CHECK(std::same_as<fr::MpAt<LongList, 21>, const B&>);
	}
}

TEST_CASE("mp_at_idx_is", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_at_idx_is<List, 0, int>);
	STATIC_CHECK_FALSE(fr::mp_at_idx_is<List, 1, int>);
	STATIC_CHECK(fr::mp_at_idx_is<List, 1, A>);
	STATIC_CHECK(fr::mp_at_idx_is<List, 2, double>);
	STATIC_CHECK(fr::mp_at_idx_is<List, 3, B>);
	STATIC_CHECK_FALSE(fr::mp_at_idx_is<List, 3, A>);

	STATIC_CHECK(fr::mp_at_idx_is<fr::MpList<int>, 0, int>);
}

TEST_CASE("MpPackAt", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<fr::MpPackAt<0, int, short&, const char, short&, void>, int>);
	STATIC_CHECK(std::same_as<fr::MpPackAt<1, int, short&, const char, short&, void>, short&>);
	STATIC_CHECK(std::same_as<fr::MpPackAt<2, int, short&, const char, short&, void>, const char>);
	STATIC_CHECK(std::same_as<fr::MpPackAt<3, int, short&, const char, short&, void>, short&>);
	STATIC_CHECK(std::same_as<fr::MpPackAt<4, int, short&, const char, short&, void>, void>);
}

TEST_CASE("mp_pack_at", "[u][engine][core][meta]") {
	auto c = 'c';
	const auto i = 34;
	CHECK(fr::mp_pack_at<0>(i, 23.f, c) == i);
	CHECK(fr::mp_pack_at<1>(i, 23.f, c) == 23.f);
	CHECK(fr::mp_pack_at<2>(i, 23.f, c) == c);
	CHECK(std::same_as<decltype(fr::mp_pack_at<0>(i, 23.f, c)), const int&>);
	CHECK(std::same_as<decltype(fr::mp_pack_at<2>(i, 23.f, c)), char&>);
	CHECK(std::same_as<decltype(fr::mp_pack_at<2>(i, 23.f, std::move(c))), char&&>);
}

TEST_CASE("mp_find", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::MpFind<List, A>::value == 1);
	STATIC_CHECK(fr::mp_find<List, int> == 0);
	STATIC_CHECK(fr::mp_find<List, B> == 3);
	STATIC_CHECK(fr::mp_find<fr::MpList<A>, A> == 0);
	STATIC_CHECK(fr::mp_find<List, char> == fr::npos);
	STATIC_CHECK(fr::mp_find<fr::MpList<>, char> == fr::npos);
}

TEST_CASE("mp_contains", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_contains<List, int>);
	STATIC_CHECK(fr::mp_contains<List, A>);
	STATIC_CHECK(fr::mp_contains<List, double>);
	STATIC_CHECK(fr::mp_contains<List, B>);

	STATIC_CHECK_FALSE(fr::mp_contains<List, char>);
	STATIC_CHECK_FALSE(fr::mp_contains<List, C>);
	STATIC_CHECK_FALSE(fr::mp_contains<List, List>);
	STATIC_CHECK_FALSE(fr::mp_contains<fr::MpList<>, C>);
	STATIC_CHECK_FALSE(fr::mp_contains<fr::MpList<>, int>);
}

TEST_CASE("mp_pack_contains", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_pack_contains<int, int, A, double, B, int, int, B>);
	STATIC_CHECK(fr::mp_pack_contains<A, int, A, double, B, int, int, B>);
	STATIC_CHECK(fr::mp_pack_contains<double, int, A, double, B, int, int, B>);
	STATIC_CHECK(fr::mp_pack_contains<B, int, A, double, B, int, int, B>);

	STATIC_CHECK_FALSE(fr::mp_pack_contains<char>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains<C, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains<List, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains<C>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains<int>);
}

TEST_CASE("mp_contains_once", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_contains_once<List, A>);
	STATIC_CHECK(fr::mp_contains_once<List, double>);

	STATIC_CHECK_FALSE(fr::mp_contains_once<List, int>);
	STATIC_CHECK_FALSE(fr::mp_contains_once<List, B>);
	STATIC_CHECK_FALSE(fr::mp_contains_once<List, char>);
	STATIC_CHECK_FALSE(fr::mp_contains_once<List, C>);
	STATIC_CHECK_FALSE(fr::mp_contains_once<List, List>);
	STATIC_CHECK_FALSE(fr::mp_contains_once<fr::MpList<>, C>);
	STATIC_CHECK_FALSE(fr::mp_contains_once<fr::MpList<>, int>);
}

TEST_CASE("mp_pack_contains_once", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_pack_contains_once<A, int, A, double, B, int, int, B>);
	STATIC_CHECK(fr::mp_pack_contains_once<double, int, A, double, B, int, int, B>);

	STATIC_CHECK_FALSE(fr::mp_pack_contains_once<int, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains_once<B, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains_once<char, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains_once<C, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains_once<List, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains_once<C, int, A, double, B, int, int, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_contains_once<int, int, A, double, B, int, int, B>);
}

TEST_CASE("mp_count", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_count<fr::MpList<int, A, int, A, A, B>, int> == 2);
	STATIC_CHECK(fr::mp_count<std::tuple<int, A, int, A, A, B>, A> == 3);
	STATIC_CHECK(fr::mp_count<std::tuple<int, A, int, A, A, B>, B> == 1);
	STATIC_CHECK(fr::mp_count<std::tuple<int, A, int, A, A, B>, C> == 0);
	STATIC_CHECK(fr::mp_count<std::tuple<>, int> == 0);
}

TEST_CASE("mp_pack_count", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_pack_count<int, int, A, int, A, A, B> == 2);
	STATIC_CHECK(fr::mp_pack_count<A, int, A, int, A, A, B> == 3);
	STATIC_CHECK(fr::mp_pack_count<B, int, A, int, A, A, B> == 1);
	STATIC_CHECK(fr::mp_pack_count<C, int, A, int, A, A, B> == 0);
	STATIC_CHECK(fr::mp_pack_count<int> == 0);
}

TEST_CASE("mp_is_unique", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_is_unique<std::tuple<int, double, A>>);
	STATIC_CHECK(fr::mp_is_unique<fr::MpList<int, int&, int&&, const int&>>);
	STATIC_CHECK(fr::mp_is_unique<std::variant<int>>);
	STATIC_CHECK(fr::mp_is_unique<std::tuple<>>);

	STATIC_CHECK_FALSE(fr::mp_is_unique<std::pair<int, int>>);
	STATIC_CHECK_FALSE(fr::mp_is_unique<fr::MpList<double, A, int, A, B>>);
	STATIC_CHECK_FALSE(fr::mp_is_unique<std::tuple<A, B, A, B>>);
}

TEST_CASE("mp_pack_is_unique", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_pack_is_unique<int, double, A>);
	STATIC_CHECK(fr::mp_pack_is_unique<int, int&, int&&, const int&>);
	STATIC_CHECK(fr::mp_pack_is_unique<int>);
	STATIC_CHECK(fr::mp_pack_is_unique<>);

	STATIC_CHECK_FALSE(fr::mp_pack_is_unique<int, int>);
	STATIC_CHECK_FALSE(fr::mp_pack_is_unique<double, A, int, A, B>);
	STATIC_CHECK_FALSE(fr::mp_pack_is_unique<A, B, A, B>);
}

TEST_CASE("mp_all_of", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_all_of<fr::MpList<>, std::is_integral>);
	STATIC_CHECK(fr::mp_all_of<std::tuple<int>, std::is_integral>);
	STATIC_CHECK(fr::mp_all_of<std::tuple<int, char>, std::is_integral>);
	STATIC_CHECK(fr::mp_all_of<std::variant<int, char, long>, std::is_integral>);
	STATIC_CHECK(fr::mp_all_of<std::tuple<int, char, long, unsigned, int>, std::is_integral>);

	STATIC_CHECK_FALSE(fr::mp_all_of<std::tuple<float>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_all_of<std::tuple<int, float>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_all_of<std::variant<float, int>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_all_of<std::variant<float, float>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_all_of<std::tuple<float, unsigned, double, int>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_all_of<std::tuple<char, unsigned, double, int, int>,
		std::is_integral>);

	STATIC_CHECK(std::same_as<
		fr::MpAllOf<std::tuple<int, char, long, unsigned, int>, std::is_integral>,
		fr::TrueC
	>);
	STATIC_CHECK(std::same_as<
		fr::MpAllOf<std::tuple<char, unsigned, double, int, int>, std::is_integral>,
		fr::FalseC
	>);
}

TEST_CASE("mp_none_of", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_none_of<std::tuple<>, std::is_integral>);
	STATIC_CHECK(fr::mp_none_of<std::tuple<float>, std::is_integral>);
	STATIC_CHECK(fr::mp_none_of<std::tuple<float, double>, std::is_integral>);
	STATIC_CHECK(fr::mp_none_of<std::variant<A, B, C>, std::is_integral>);
	STATIC_CHECK(fr::mp_none_of<std::variant<float, double, A, B, C>, std::is_integral>);

	STATIC_CHECK_FALSE(fr::mp_none_of<std::tuple<int>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_none_of<std::tuple<int, float>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_none_of<std::variant<float, int>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_none_of<std::variant<int, int>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_none_of<std::tuple<int, int>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_none_of<std::tuple<float, double, int, long double>,
		std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_none_of<std::tuple<int, float, double, int, long double>,
		std::is_integral>);

	STATIC_CHECK(std::same_as<
		fr::MpNoneOf<std::variant<float, double, A, B, C>, std::is_integral>,
		fr::TrueC
	>);
	STATIC_CHECK(std::same_as<
		fr::MpNoneOf<std::variant<int, float, double, int, long double>, std::is_integral>,
		fr::FalseC
	>);
}

TEST_CASE("mp_any_of", "[u][engine][core][meta]") {
	STATIC_CHECK(fr::mp_any_of<std::tuple<int>, std::is_integral>);
	STATIC_CHECK(fr::mp_any_of<std::tuple<int, float>, std::is_integral>);
	STATIC_CHECK(fr::mp_any_of<std::variant<float, char>, std::is_integral>);
	STATIC_CHECK(fr::mp_any_of<std::variant<char, unsigned>, std::is_integral>);
	STATIC_CHECK(fr::mp_any_of<std::variant<A, float, B, double, C, unsigned>, std::is_integral>);
	STATIC_CHECK(fr::mp_any_of<std::variant<int, char, unsigned, unsigned>, std::is_integral>);

	STATIC_CHECK_FALSE(fr::mp_any_of<std::tuple<>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_any_of<std::variant<float, float>, std::is_integral>);
	STATIC_CHECK_FALSE(fr::mp_any_of<std::variant<float, A, B, C, double>, std::is_integral>);

	STATIC_CHECK(std::same_as<
		fr::MpAnyOf<std::variant<int, float, double, int, long double>, std::is_integral>,
		fr::TrueC
	>);
	STATIC_CHECK(std::same_as<
		fr::MpAnyOf<std::variant<float, double, A, B, C>, std::is_integral>,
		fr::FalseC
	>);
}

TEST_CASE("MpRepack", "[u][engine][core][meta]") {
	SECTION("empty") {
		STATIC_CHECK(std::same_as<
			fr::MpRepack<std::variant<>, fr::MpConcat>,
			fr::MpList<>
		>);
	}
	SECTION("non-empty") {
		STATIC_CHECK(std::same_as<
			fr::MpRepack<
				fr::MpList<
					fr::MpValueList<4, 'a'>,
					fr::MpValueList<45.f>,
					fr::MpValueList<>,
					fr::MpValueList<A{}, B{}, 5>
				>,
				fr::MpConcat
			>,
			fr::MpValueList<4, 'a', 45.f, A{}, B{}, 5>
		>);
	}
}

TEST_CASE("MpConcat.types", "[u][engine][core][meta]") {
	SECTION("nothing") {
		STATIC_CHECK(std::same_as<fr::MpConcat<>, fr::MpList<>>);
	}
	SECTION("single empty list") {
		STATIC_CHECK(std::same_as<fr::MpConcat<fr::MpList<>>, fr::MpList<>>);
	}
	SECTION("single list") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<std::tuple<int, float>>,
			std::tuple<int, float>
		>);
	}
	SECTION("two lists") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<
				std::tuple<char, int, std::string>,
				std::tuple<float, char>
			>,
			std::tuple<char, int, std::string, float, char>
		>);
	}
	SECTION("three lists") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<
				fr::MpList<char, int, std::string>,
				fr::MpList<double>,
				fr::MpList<float, char>
			>,
			fr::MpList<char, int, std::string, double, float, char>
		>);
	}
	SECTION("four lists") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<
				std::tuple<char, int, std::string>,
				std::tuple<double>,
				std::tuple<float, char>,
				std::tuple<std::vector<short>, short>
			>,
			std::tuple<char, int, std::string, double, float, char, std::vector<short>, short>
		>);
	}
}

TEST_CASE("MpConcat.values", "[u][engine][core][meta]") {
	SECTION("nothing") {
		STATIC_CHECK(std::same_as<fr::MpConcat<>, fr::MpList<>>);
	}
	SECTION("single empty list") {
		STATIC_CHECK(std::same_as<fr::MpConcat<fr::MpValueList<>>, fr::MpValueList<>>);
	}
	SECTION("single list") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<fr::MpValueList<int{2}, float{1.f}>>,
			fr::MpValueList<int{2}, float{1.f}>
		>);
	}
	SECTION("two lists") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<
				fr::MpValueList<'a', 3, fr::StringLiteral{"abc"}>,
				fr::MpValueList<-1.f, 'Z'>
			>,
			fr::MpValueList<'a', 3, fr::StringLiteral{"abc"}, -1.f, 'Z'>
		>);
	}
	SECTION("three lists") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<
				fr::MpValueList<'a', 3, fr::StringLiteral{"abc"}>,
				fr::MpValueList<4.>,
				fr::MpValueList<-1.f, 'Z'>
			>,
			fr::MpValueList<'a', 3, fr::StringLiteral{"abc"}, 4., -1.f, 'Z'>
		>);
	}
	SECTION("four lists") {
		STATIC_CHECK(std::same_as<
			fr::MpConcat<
				fr::MpValueList<'a', 3, fr::StringLiteral{"abc"}>,
				fr::MpValueList<4.>,
				fr::MpValueList<A{}, B{}>,
				fr::MpValueList<-1.f, 'Z'>
			>,
			fr::MpValueList<'a', 3, fr::StringLiteral{"abc"}, 4., A{}, B{}, -1.f, 'Z'>
		>);
	}
}

TEST_CASE("MpTransform", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpTransform<fr::MpList<>, std::add_pointer_t>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpTransform<fr::MpList<int&>, std::add_pointer_t>,
		fr::MpList<int*>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpTransform<std::tuple<int, const double, void*>, std::add_pointer_t>,
		std::tuple<int*, const double*, void**>
	>);
}

TEST_CASE("MpFlatten", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<fr::MpFlatten<fr::MpList<>>, fr::MpList<>>);
	STATIC_CHECK(std::same_as<fr::MpFlatten<fr::MpList<int>>, fr::MpList<int>>);
	STATIC_CHECK(std::same_as<fr::MpFlatten<fr::MpList<int, fr::MpList<>>>, fr::MpList<int>>);
	STATIC_CHECK(std::same_as<fr::MpFlatten<fr::MpList<fr::MpList<>, int>>, fr::MpList<int>>);
	STATIC_CHECK(std::same_as<fr::MpFlatten<fr::MpList<fr::MpList<int>>>, fr::MpList<int>>);

	STATIC_CHECK(std::same_as<
		fr::MpFlatten<fr::MpList<fr::MpList<int>, fr::MpList<char>>>,
		fr::MpList<int, char>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFlatten<fr::MpList<fr::MpList<int>, fr::MpList<char>, fr::MpList<double>>>,
		fr::MpList<int, char, double>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFlatten<fr::MpList<fr::MpList<int, char>, fr::MpList<double>>>,
		fr::MpList<int, char, double>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFlatten<fr::MpList<fr::MpList<int>, fr::MpList<char, double>>>,
		fr::MpList<int, char, double>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFlatten<std::tuple<std::tuple<int>, std::tuple<char, double>>>,
		std::tuple<int, char, double>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFlatten<fr::MpList<fr::MpList<int>, std::tuple<char, double>>>,
		fr::MpList<int, std::tuple<char, double>>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFlatten<fr::MpList<fr::MpList<int, fr::MpList<char>>, fr::MpList<void>>>,
		fr::MpList<int, char, void>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFlatten<std::tuple<
			std::tuple<int, std::tuple<std::tuple<char, double>>>,
			std::tuple<void>,
			unsigned,
			std::tuple<>,
			std::tuple<std::variant<std::string, std::tuple<short>>>,
			std::tuple<std::string, bool, char, char, void, long>
		>>,
		std::tuple<int, char, double, void, unsigned, std::variant<std::string, std::tuple<short>>,
			std::string, bool, char,  char, void, long>
	>);
}

template<class T>
using IsIntegral = fr::BoolC<std::is_integral_v<T>>;

TEST_CASE("MpRemoveIf", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIf<fr::MpList<>, IsIntegral>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIf<fr::MpList<float>, IsIntegral>,
		fr::MpList<float>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIf<fr::MpList<int, size_t>, IsIntegral>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIf<
			fr::MpList<int, std::string, float, A, B, A, unsigned, double, int, C, char, C>,
			IsIntegral
		>,
		fr::MpList<std::string, float, A, B, A, double, C, C>
	>);
}

TEST_CASE("MpFilter", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpFilter<fr::MpList<>, IsIntegral>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFilter<fr::MpList<int>, IsIntegral>,
		fr::MpList<int>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFilter<fr::MpList<float, double>, IsIntegral>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFilter<
			fr::MpList<int, A, int, std::string, A, float, B, unsigned, int, char>,
			IsIntegral
		>,
		fr::MpList<int, int, unsigned, int, char>
	>);
}

TEST_CASE("MpRemoveIfProj", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIfProj<fr::MpList<>, IsIntegral, std::remove_pointer_t>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIfProj<fr::MpList<int*>, IsIntegral, std::remove_pointer_t>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIfProj<fr::MpList<int*, double&>, IsIntegral, std::remove_pointer_t>,
		fr::MpList<double&>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIfProj<
			fr::MpList<int, A, int, std::string, A, float, B, unsigned, int, char>,
			IsIntegral,
			std::type_identity_t
		>,
		fr::MpList<A, std::string, A, float, B>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpRemoveIfProj<
			fr::MpList<int&, A, int, std::string*, A&, float&, B, unsigned&, int&, char>,
			IsIntegral,
			std::remove_reference_t
		>,
		fr::MpList<A, std::string*, A&, float&, B>
	>);
}

TEST_CASE("MpFilterProj", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpFilterProj<fr::MpList<>, IsIntegral, std::remove_pointer_t>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFilterProj<fr::MpList<int*>, IsIntegral, std::remove_pointer_t>,
		fr::MpList<int*>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFilterProj<fr::MpList<float*, double&>, IsIntegral, std::remove_pointer_t>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFilterProj<
			fr::MpList<int, A, int, std::string, A, float, B, unsigned, int, char>,
			IsIntegral,
			std::type_identity_t
		>,
		fr::MpList<int, int, unsigned, int, char>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFilterProj<
			fr::MpList<int&, A, int, std::string*, A&, float&, B, unsigned&, int&, char>,
			IsIntegral,
			std::remove_reference_t
		>,
		fr::MpList<int&, int, unsigned&, int&, char>
	>);
}

TEST_CASE("MpEnumerate", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpEnumerate<fr::MpList<>>,
		fr::MpList<>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpEnumerate<std::tuple<A>>,
		std::tuple<fr::MpIndexedType<0zu, A>>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpEnumerate<std::variant<char*, A, int, int, B, void, A>>,
		std::variant<
			fr::MpIndexedType<0zu, char*>,
			fr::MpIndexedType<1zu, A>,
			fr::MpIndexedType<2zu, int>,
			fr::MpIndexedType<3zu, int>,
			fr::MpIndexedType<4zu, B>,
			fr::MpIndexedType<5zu, void>,
			fr::MpIndexedType<6zu, A>
		>
	>);
}

TEST_CASE("MpFirst", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpFirst<fr::MpList<float*>>,
		float*
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFirst<fr::MpList<float*, double, A, B>>,
		float*
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFirst<fr::MpIndexedType<3, A>>,
		fr::ValueC<3zu>
	>);
	STATIC_CHECK(std::same_as<
		fr::MpFirst<fr::MpValueList<'a', 'b', 'c', 'd'>>,
		fr::ValueC<'a'>
	>);
}

TEST_CASE("MpSecond", "[u][engine][core][meta]") {
	STATIC_CHECK(std::same_as<
		fr::MpSecond<fr::MpList<float*, void>>,
		void
	>);
	STATIC_CHECK(std::same_as<
		fr::MpSecond<fr::MpList<float*, double, A, B>>,
		double
	>);
	STATIC_CHECK(std::same_as<
		fr::MpSecond<fr::MpIndexedType<3, A>>,
		A
	>);
	STATIC_CHECK(std::same_as<
		fr::MpSecond<fr::MpValueList<'a', 'b', 'c', 'd'>>,
		fr::ValueC<'b'>
	>);
}

TEST_CASE("for_each_type", "[u][engine][core][meta]") {
	SECTION("empty list") {
		bool flag = false;
		fr::for_each_type<fr::MpList<>>([&]<class> {
			flag = true;
		});
		CHECK_FALSE(flag);
	}
	SECTION("two types") {
		bool flags[2] = {};
		fr::for_each_type<fr::MpList<char, unsigned>>([&]<class T> {
			if constexpr (std::is_same_v<T, char>)
				flags[0] = true;
			else if constexpr (std::is_same_v<T, unsigned>)
				flags[1] = true;
			else
				static_assert(fr::always_false<T>);
		});
		for (auto flag : flags)
			CHECK(flag);
	}
	SECTION("three types") {
		bool flags[3] = {};
		fr::for_each_type<fr::MpList<long, double, std::string>>([&]<class T> {
			if constexpr (std::is_same_v<T, long>)
				flags[0] = true;
			else if constexpr (std::is_same_v<T, double>)
				flags[1] = true;
			else if constexpr (std::is_same_v<T, std::string>)
				flags[2] = true;
			else
				static_assert(fr::always_false<T>);
		});
		for (auto flag : flags)
			CHECK(flag);
	}
}

TEST_CASE("unroll", "[u][engine][core][meta]") {
	SECTION("zero iterations") {
		auto i = 0zu;
		fr::unroll<0>([&]<size_t I> { ++i; });
		CHECK(i == 0zu);
	}
	SECTION("one iteration") {
		auto i = 0zu;
		fr::unroll<0>([&]<size_t I> {
			CHECK(I == i);
			++i;
		});
		CHECK(i == 0zu);
	}
	SECTION("many iterations") {
		auto i = 0zu;
		fr::unroll<5>([&]<size_t I> {
			CHECK(I == i);
			++i;
		});
		CHECK(i == 5zu);
	}
	SECTION("canceled iteration") {
		auto i = 0zu;
		fr::unroll<5>([&]<size_t I> {
			CHECK(I == i);
			++i;
			return i != 2zu;
		});
		CHECK(i == 2zu);
	}
}
