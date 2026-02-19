#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/range_concepts.hpp"

#include <any>
#include <array>
#include <complex>
#include <deque>
#include <forward_list>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace {

struct Dummy { };

struct NoDefaultCtor {
	NoDefaultCtor() = delete;
	NoDefaultCtor(int, char) { }
};

template<bool IsExplicit, bool IsNoexcept>
struct CtorConvert {
	explicit(IsExplicit)
	CtorConvert(Dummy) noexcept(IsNoexcept) { }
};

template<bool IsExplicit, bool IsNoexcept>
struct OpConvert {
	explicit(IsExplicit)
	operator Dummy() const noexcept(IsNoexcept) { return {}; }
};

} // namespace

// concepts.hpp
// ============

TEST_CASE("c_explicitly_convertible_to") {
	SECTION("operator conversion") {
		STATIC_CHECK(fr::c_explicitly_convertible_to<void, void>);
		STATIC_CHECK(fr::c_explicitly_convertible_to<volatile void, const void>);

		STATIC_CHECK(fr::c_explicitly_convertible_to<OpConvert<true, true>, Dummy>);
		STATIC_CHECK(fr::c_explicitly_convertible_to<OpConvert<true, false>, Dummy>);

		STATIC_CHECK(fr::c_explicitly_convertible_to<OpConvert<false, true>, Dummy>);
		STATIC_CHECK(fr::c_explicitly_convertible_to<OpConvert<false, false>, Dummy>);

		STATIC_CHECK_FALSE(fr::c_explicitly_convertible_to<OpConvert<false, true>, int>);
		STATIC_CHECK_FALSE(fr::c_explicitly_convertible_to<Dummy, OpConvert<false, true>>);
	}
	SECTION("constructor conversion") {
		STATIC_CHECK(fr::c_explicitly_convertible_to<Dummy, CtorConvert<true, true>>);
		STATIC_CHECK(fr::c_explicitly_convertible_to<Dummy, CtorConvert<true, false>>);

		STATIC_CHECK(fr::c_explicitly_convertible_to<Dummy, CtorConvert<false, true>>);
		STATIC_CHECK(fr::c_explicitly_convertible_to<Dummy, CtorConvert<false, false>>);

		STATIC_CHECK_FALSE(fr::c_explicitly_convertible_to<CtorConvert<false, true>, int>);
		STATIC_CHECK_FALSE(fr::c_explicitly_convertible_to<CtorConvert<false, true>, Dummy>);
	}
}

TEST_CASE("c_nothrow_explicitly_convertible_to") {
	SECTION("operator conversion") {
		STATIC_CHECK(fr::c_nothrow_explicitly_convertible_to<void, void>);
		STATIC_CHECK(fr::c_nothrow_explicitly_convertible_to<volatile void, const void>);

		STATIC_CHECK(fr::c_nothrow_explicitly_convertible_to<OpConvert<true, true>, Dummy>);
		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<OpConvert<true, false>, Dummy>);

		STATIC_CHECK(fr::c_nothrow_explicitly_convertible_to<OpConvert<false, true>, Dummy>);
		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<OpConvert<false, false>, Dummy>);

		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<OpConvert<false, true>, int>);
		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<Dummy, OpConvert<false, true>>);
	}
	SECTION("constructor conversion") {
		STATIC_CHECK(fr::c_nothrow_explicitly_convertible_to<Dummy, CtorConvert<true, true>>);
		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<Dummy,
			CtorConvert<true, false>>);

		STATIC_CHECK(fr::c_nothrow_explicitly_convertible_to<Dummy, CtorConvert<false, true>>);
		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<Dummy,
			CtorConvert<false, false>>);

		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<CtorConvert<false, true>, int>);
		STATIC_CHECK_FALSE(fr::c_nothrow_explicitly_convertible_to<CtorConvert<false, true>,
			Dummy>);
	}
}

// range_concepts.hpp
// ==================

TEST_CASE("c_constexpr_sized_range", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_constexpr_sized_range<std::array<int, 5>>);
	STATIC_CHECK(fr::c_constexpr_sized_range<std::array<std::unique_ptr<int>, 5>>);
	STATIC_CHECK(fr::c_constexpr_sized_range<std::array<NoDefaultCtor, 5>>);
	STATIC_CHECK(fr::c_constexpr_sized_range<std::span<int, 15>>);
	STATIC_CHECK(fr::c_constexpr_sized_range<std::span<NoDefaultCtor, 5>>);

	STATIC_CHECK_FALSE(fr::c_constexpr_sized_range<std::span<NoDefaultCtor>>);
	STATIC_CHECK_FALSE(fr::c_constexpr_sized_range<std::vector<int>>);
}

TEST_CASE("constexpr_size", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::constexpr_size<std::array<int, 23>> == 23);
	STATIC_CHECK(fr::constexpr_size<float[8]> == 8);
	STATIC_CHECK(fr::constexpr_size<std::span<char, 50>> == 50);
}

TEST_CASE("c_constexpr_sized_container", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_constexpr_sized_container<std::array<int, 23>>);
	STATIC_CHECK(fr::c_constexpr_sized_container<std::array<std::string, 0>>);

	STATIC_CHECK_FALSE(fr::c_constexpr_sized_container<std::vector<float>>);
}

TEST_CASE("c_std_array_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_std_array_like<std::array<int, 5>>);
	STATIC_CHECK(fr::c_std_array_like<std::array<int, 0>>);
	STATIC_CHECK(fr::c_std_array_like<std::array<std::string, 5>>);
	STATIC_CHECK(fr::c_std_array_like<std::array<NoDefaultCtor, 5>>);

	STATIC_CHECK_FALSE(fr::c_std_array_like<int[20]>);
	STATIC_CHECK_FALSE(fr::c_std_array_like<int[]>);
	STATIC_CHECK_FALSE(fr::c_std_array_like<std::span<int>>);
	STATIC_CHECK_FALSE(fr::c_std_array_like<std::span<int, 5>>);
	STATIC_CHECK_FALSE(fr::c_std_array_like<std::span<NoDefaultCtor, 5>>);
	STATIC_CHECK_FALSE(fr::c_std_array_like<std::vector<NoDefaultCtor>>);
}

TEST_CASE("c_array_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_array_like<std::array<int, 5>>);
	STATIC_CHECK(fr::c_array_like<std::array<int, 0>>);
	STATIC_CHECK(fr::c_array_like<std::array<std::string, 5>>);
	STATIC_CHECK(fr::c_array_like<std::array<NoDefaultCtor, 5>>);
	STATIC_CHECK(fr::c_array_like<int[20]>);
	STATIC_CHECK(fr::c_array_like<NoDefaultCtor[20]>);
	STATIC_CHECK(fr::c_array_like<std::string[20]>);

	STATIC_CHECK_FALSE(fr::c_array_like<int[]>);
	STATIC_CHECK_FALSE(fr::c_array_like<std::span<int>>);
	STATIC_CHECK_FALSE(fr::c_array_like<std::span<int, 5>>);
	STATIC_CHECK_FALSE(fr::c_array_like<std::span<NoDefaultCtor, 5>>);
	STATIC_CHECK_FALSE(fr::c_array_like<std::vector<NoDefaultCtor>>);
	STATIC_CHECK_FALSE(fr::c_array_like<std::initializer_list<int>>);
}

TEST_CASE("c_list_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_list_like<std::list<int>>);
	STATIC_CHECK(fr::c_list_like<std::list<NoDefaultCtor>>);
	STATIC_CHECK(fr::c_list_like<std::list<std::vector<std::string>>>);

	STATIC_CHECK_FALSE(fr::c_list_like<std::vector<int>>);
	STATIC_CHECK_FALSE(fr::c_list_like<std::deque<int>>);
	STATIC_CHECK_FALSE(fr::c_list_like<int[20]>);
	STATIC_CHECK_FALSE(fr::c_list_like<std::vector<std::list<int>>>);
	STATIC_CHECK_FALSE(fr::c_list_like<std::forward_list<int>>);
}

TEST_CASE("c_vector_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_vector_like<std::vector<std::string>>);
	STATIC_CHECK(fr::c_vector_like<std::vector<NoDefaultCtor>>);
	STATIC_CHECK(fr::c_vector_like<std::vector<int>>);

	STATIC_CHECK_FALSE(fr::c_vector_like<std::list<int>>);
	STATIC_CHECK_FALSE(fr::c_vector_like<std::deque<char>>);
	STATIC_CHECK_FALSE(fr::c_vector_like<std::string>);
	STATIC_CHECK_FALSE(fr::c_vector_like<std::array<int, 5>>);
}

TEST_CASE("c_string_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_string_like<std::string>);
	STATIC_CHECK(fr::c_string_like<std::wstring>);
	STATIC_CHECK(fr::c_string_like<std::u8string>);
	STATIC_CHECK(fr::c_string_like<std::u16string>);
	STATIC_CHECK(fr::c_string_like<std::u32string>);

	STATIC_CHECK_FALSE(fr::c_string_like<std::string_view>);
	STATIC_CHECK_FALSE(fr::c_string_like<std::basic_string_view<char16_t>>);

	STATIC_CHECK_FALSE(fr::c_string_like<std::span<char>>);
	STATIC_CHECK_FALSE(fr::c_string_like<std::span<const char>>);
	STATIC_CHECK_FALSE(fr::c_string_like<std::span<int>>);
	STATIC_CHECK_FALSE(fr::c_string_like<std::vector<char>>);
	STATIC_CHECK_FALSE(fr::c_string_like<std::vector<char32_t>>);
}

TEST_CASE("c_span_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_span_like<std::span<int>>);
	STATIC_CHECK(fr::c_span_like<std::span<int, 5>>);
	STATIC_CHECK(fr::c_span_like<std::span<NoDefaultCtor, 5>>);

	STATIC_CHECK_FALSE(fr::c_span_like<std::vector<NoDefaultCtor>>);
	STATIC_CHECK_FALSE(fr::c_span_like<int[5]>);
	STATIC_CHECK_FALSE(fr::c_span_like<int[]>);
	STATIC_CHECK_FALSE(fr::c_span_like<std::array<int, 5>>);
	STATIC_CHECK_FALSE(fr::c_span_like<std::array<std::string, 5>>);
	STATIC_CHECK_FALSE(fr::c_span_like<std::array<NoDefaultCtor, 5>>);
}

TEST_CASE("c_dynamic_extent_span_like", "[u][engine][core][concepts]") {
	STATIC_CHECK_FALSE(fr::c_dynamic_extent_span_like<std::span<int, 5>>);
	STATIC_CHECK_FALSE(fr::c_dynamic_extent_span_like<std::span<std::string, 5>>);
	STATIC_CHECK_FALSE(fr::c_dynamic_extent_span_like<std::span<NoDefaultCtor, 5>>);
	STATIC_CHECK_FALSE(fr::c_const_extent_span_like<std::vector<int>>);

	STATIC_CHECK(fr::c_dynamic_extent_span_like<std::span<int>>);
	STATIC_CHECK(fr::c_dynamic_extent_span_like<std::span<char>>);
}

TEST_CASE("c_const_extent_span_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_const_extent_span_like<std::span<int, 5>>);
	STATIC_CHECK(fr::c_const_extent_span_like<std::span<std::string, 5>>);
	STATIC_CHECK(fr::c_const_extent_span_like<std::span<NoDefaultCtor, 5>>);

	STATIC_CHECK_FALSE(fr::c_const_extent_span_like<std::span<int>>);
	STATIC_CHECK_FALSE(fr::c_const_extent_span_like<std::vector<int>>);
}

TEST_CASE("c_string_view_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_string_view_like<std::string_view>);
	STATIC_CHECK(fr::c_string_view_like<std::basic_string_view<char16_t>>);
	STATIC_CHECK(fr::c_string_view_like<std::basic_string_view<char32_t>>);
	STATIC_CHECK(fr::c_string_view_like<std::basic_string_view<wchar_t>>);

	STATIC_CHECK_FALSE(fr::c_string_view_like<std::span<char>>);
	STATIC_CHECK_FALSE(fr::c_string_view_like<std::span<const char>>);
	STATIC_CHECK_FALSE(fr::c_string_view_like<std::span<int>>);
	STATIC_CHECK_FALSE(fr::c_string_view_like<std::string>);
	STATIC_CHECK_FALSE(fr::c_string_view_like<std::wstring>);
}

TEST_CASE("c_optional_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_optional_like<std::optional<int>>);
	STATIC_CHECK(fr::c_optional_like<std::optional<std::string>>);
	STATIC_CHECK(fr::c_optional_like<std::optional<std::optional<int>>>);
	STATIC_CHECK(fr::c_optional_like<std::optional<std::unique_ptr<int>>>);

	STATIC_CHECK_FALSE(fr::c_optional_like<int>);
	STATIC_CHECK_FALSE(fr::c_optional_like<std::variant<int, std::monostate>>);
	STATIC_CHECK_FALSE(fr::c_optional_like<std::any>);
	STATIC_CHECK_FALSE(fr::c_optional_like<std::string*>);
}

TEST_CASE("c_tuple_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_tuple_like<std::tuple<int, float, char>>);
	STATIC_CHECK(fr::c_tuple_like<std::pair<int, float>>);
	STATIC_CHECK(fr::c_tuple_like<std::array<int, 4>>);
	STATIC_CHECK(fr::c_aggregate<int[4]>);
#if 0 // TODO: Enable in C++26
	STATIC_CHECK(fr::c_tuple_like<std::complex<float>>);
#endif

	STATIC_CHECK_FALSE(fr::c_tuple_like<std::variant<int, std::monostate>>);
	STATIC_CHECK_FALSE(fr::c_tuple_like<std::any>);
	STATIC_CHECK_FALSE(fr::c_tuple_like<std::vector<std::string>>);
	STATIC_CHECK_FALSE(fr::c_tuple_like<int>);
}

TEST_CASE("c_record_like", "[u][engine][core][concepts]") {
	STATIC_CHECK(fr::c_record_like<std::array<int, 4>>);
	STATIC_CHECK(fr::c_record_like<std::tuple<int, float>>);
	STATIC_CHECK(fr::c_record_like<Dummy>);
}
