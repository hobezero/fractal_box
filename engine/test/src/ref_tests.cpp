#include "fractal_box/core/ref.hpp"

#include <concepts>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "test_common/test_helpers.hpp"

namespace {

struct Base { };
struct Derived: Base { };

struct WithAddressOf {
	struct Dummy { };
	auto operator&() const noexcept -> Dummy { return {}; }
};

} // namespace

TEST_CASE("Ref", "[u][engine][core][ref]") {
	SECTION("special member functions") {
		STATIC_CHECK_FALSE(std::is_default_constructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_copy_constructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_nothrow_copy_constructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_copy_assignable_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_nothrow_copy_assignable_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_move_constructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_move_assignable_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_destructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_nothrow_destructible_v<fr::Ref<int>>);
	}
	SECTION("triviality") {
		STATIC_CHECK(std::is_trivially_copyable_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_trivially_copy_constructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_trivially_move_constructible_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_trivially_copy_assignable_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_trivially_move_assignable_v<fr::Ref<int>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::Ref<int>>);
	}
	SECTION("constructors") {
		// TODO: Find a way to test whether Ref is constructible from an rvalue without causing
		// a compilation error. Unfortunately, SFINAE-based tricks (`std::is_constructible`,
		// `std::constructible_from`, custom concepts) don't seem to work because substitution
		// never fails: `Ref<T>:::to_ptr(T&&)` is defined as deleted

		STATIC_CHECK(std::constructible_from<fr::Ref<int>, int&>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, int*>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, const int*>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, const int&>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, volatile int&>);

		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, char&>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, float&>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, short&>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<int>, long&>);

		STATIC_CHECK(std::constructible_from<fr::Ref<const int>, int&>);
		STATIC_CHECK(std::constructible_from<fr::Ref<const int>, const int&>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<const int>, const int*>);

		STATIC_CHECK(std::constructible_from<fr::Ref<Base>, Derived&>);
		STATIC_CHECK(std::constructible_from<fr::Ref<Base>, Base&>);
		STATIC_CHECK(std::constructible_from<fr::Ref<const Base>, Base&>);
		STATIC_CHECK_FALSE(std::constructible_from<fr::Ref<Derived>, Base&>);
		STATIC_CHECK(std::constructible_from<fr::Ref<WithAddressOf>, WithAddressOf&>);
	}
	SECTION("getters") {
		constexpr auto test_type = []<class T>() {
			auto object = T{};
			auto ref = fr::Ref<T>{object};
			{
				CHECK(std::addressof(ref.get()) == std::addressof(object));
				CHECK(std::addressof(static_cast<T&>(ref)) == std::addressof(object));
				CHECK(std::addressof(*ref) == std::addressof(object));
				CHECK(ref.operator->() == std::addressof(object));
			}
			{
				const auto& cref = std::as_const(ref);
				CHECK(std::addressof(cref.get()) == std::addressof(object));
				CHECK(std::addressof(static_cast<T&>(cref)) == std::addressof(object));
				CHECK(std::addressof(*cref) == std::addressof(object));
				CHECK(cref.operator->() == std::addressof(object));
			}
		};
		frt::typed_section<int>(test_type);
		frt::typed_section<double>(test_type);
		frt::typed_section<Base>(test_type);
		frt::typed_section<WithAddressOf>(test_type);
	}
	SECTION("assignment") {
		int num1{};
		int num2{};
		fr::Ref<int> ref(num1);
		ref = num2;
		CHECK(std::addressof(ref.get()) == std::addressof(num2));
	}
	SECTION("deduction guide") {
		float num;
		Derived obj;
		CHECK(std::same_as<decltype(fr::Ref{num}), fr::Ref<float>>);
		CHECK(std::same_as<decltype(fr::Ref{obj}), fr::Ref<Derived>>);
	}
}
