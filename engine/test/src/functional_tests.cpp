#include "fractal_box/core/function_ref.hpp"
#include "fractal_box/core/functional.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/meta/meta.hpp"
#include "test_common/test_helpers.hpp"

namespace {

struct Incomplete;

struct MyTrivial {
	unsigned foo(char, int) const volatile & noexcept;

public:
	char data;
};

struct MyTrivialLarge {
	double a;
	double b;
	double c;
	double d;
};

struct MyNonTrivial {
	~MyNonTrivial() { }
};

struct MyPred { auto operator()(int, char, long) noexcept(false) { return true; } };

// CV-qualifiers
struct MyPredC { auto operator()(int, char, long) const { return true; } };
struct MyPredV { auto operator()(int, char, long) volatile { return true; } };
struct MyPredCV { auto operator()(int, char, long) const volatile { return true; } };

// Ref-qulifiers
struct MyPredL { auto operator()(int, char, long) & { return true; } };
struct MyPredCL { auto operator()(int, char, long) const & { return true; } };
struct MyPredVL { auto operator()(int, char, long) volatile & { return true; } };
struct MyPredCVL { auto operator()(int, char, long) const volatile & { return true; } };

struct MyPredR { auto operator()(int, char, long) const && { return true; } };
struct MyPredCR { auto operator()(int, char, long) const && { return true; } };
struct MyPredVR { auto operator()(int, char, long) volatile && { return true; } };
struct MyPredCVR { auto operator()(int, char, long) const volatile && { return true; } };

// noexcept
struct MyPredN { auto operator()(int, char, long) noexcept { return true; } };
struct MyPredCN { auto operator()(int, char, long) const noexcept { return true; } };
struct MyPredVN { auto operator()(int, char, long) volatile noexcept { return true; } };
struct MyPredCVN { auto operator()(int, char, long) const volatile noexcept { return true; } };

struct MyPredLN { auto operator()(int, char, long) & noexcept { return true; } };
struct MyPredCLN { auto operator()(int, char, long) const & noexcept { return true; } };
struct MyPredVLN { auto operator()(int, char, long) volatile & noexcept { return true; } };
struct MyPredCVLN { auto operator()(int, char, long) const volatile & noexcept { return true; } };

struct MyPredRN { auto operator()(int, char, long) && noexcept { return true; } };
struct MyPredCRN { auto operator()(int, char, long) const && noexcept { return true; } };
struct MyPredVRN { auto operator()(int, char, long) volatile && noexcept { return true; } };
struct MyPredCVRN { auto operator()(int, char, long) const volatile && noexcept { return true; } };

static constexpr auto my_preds_throwable = fr::mp_list<
	MyPred, MyPredC, MyPredV ,MyPredCV,
	MyPredL, MyPredCL, MyPredVL, MyPredCVL,
	MyPredR, MyPredCR, MyPredVR, MyPredCVR
>;

static constexpr auto my_preds_noexcept = fr::mp_list<
	MyPredN, MyPredCN, MyPredVN ,MyPredCVN,
	MyPredLN, MyPredCLN, MyPredVLN, MyPredCVLN,
	MyPredRN, MyPredCRN, MyPredVRN, MyPredCVRN
>;

template<class F>
static constexpr auto is_valid_free_func = fr::false_c;

template<class Ret, class... Args>
static constexpr auto is_valid_free_func<Ret (Args...)> = fr::true_c;

template<class Ret, class... Args>
static constexpr auto is_valid_free_func<Ret (Args...) noexcept> = fr::true_c;

} // namespace

TEST_CASE("PassAbi", "[u][engine][core][functional]") {
	CHECK(std::same_as<fr::PassAbi<int>, int>);
	CHECK(std::same_as<fr::PassAbi<double>, double>);
	CHECK(std::same_as<fr::PassAbi<double*>, double*>);
	CHECK(std::same_as<fr::PassAbi<MyTrivial>, MyTrivial>);
	CHECK(std::same_as<fr::PassAbi<MyTrivialLarge>, const MyTrivialLarge&>);
	CHECK(std::same_as<fr::PassAbi<MyNonTrivial>, const MyNonTrivial&>);
}

TEST_CASE("MemberType", "[u][engine][core][functional]") {
	CHECK(std::same_as<fr::MemberType<decltype(&MyTrivial::data)>, char>);
	CHECK(std::same_as<fr::MemberType<decltype(&MyTrivial::foo)>,
		unsigned (char, int) const volatile & noexcept>);
}

TEST_CASE("MemberEqualTo", "[u][engine][core][functional]") {
	auto cmp = fr::MemberEqualTo<&MyTrivial::data>{};
	const auto a1 = MyTrivial{.data = 'a'};
	const auto a2 = MyTrivial{.data = 'a'};
	const auto b = MyTrivial{.data = 'b'};

	CHECK(cmp(a1, a1));
	CHECK(cmp(a1, a2));
	CHECK_FALSE(cmp(a1, b));
	CHECK_FALSE(cmp(b, a1));

	CHECK(cmp(a1, 'a'));
	CHECK(cmp('a', a1));

	CHECK_FALSE(cmp(a1, 'x'));
	CHECK_FALSE(cmp('x', a1));
}

TEST_CASE("FuncTraits", "[u][engine][core][functional]") {
	static const auto do_test = []<class P> {
		const auto check = []<class F> {
			using Traits = fr::FuncTraits<F>;
			CHECK(std::same_as<typename Traits::Stripped, bool (int, char, long)>);
			CHECK(std::same_as<typename Traits::Result, bool>);
			CHECK(std::same_as<typename Traits::Arguments, fr::MpList<int, char, long>>);
			if (std::is_class_v<F>)
				CHECK(Traits::kind == fr::CallableKind::Class);
			else if (std::is_function_v<std::remove_cvref_t<std::remove_pointer_t<F>>>)
				CHECK(Traits::kind == fr::CallableKind::FreeFunction);
			else
				CHECK(Traits::kind == fr::CallableKind::MemberFunction);
			// TODO: Check `Traits::qualifiers`
		};

		using FreeType = fr::MemberType<decltype(&P::operator())>;
		static_assert(std::is_function_v<FreeType>);
		frt::named_typed_section<FreeType>("free function", check);
		if constexpr (is_valid_free_func<FreeType>) {
			frt::named_typed_section<FreeType*>("pointer to free function", check);
			frt::named_typed_section<FreeType&>("lvalue reference to free function", check);
			frt::named_typed_section<FreeType&&>("rvalue reference to free function", check);
		}
		frt::named_typed_section<P>("function object", check);
		frt::named_typed_section<decltype(&P::operator())>("member function pointer", check);
	};

	SECTION("throwable") {
		fr::for_each_type(my_preds_throwable, []<class Pred> {
			do_test.template operator()<Pred>();
		});
	}
	SECTION("noexcept") {
		fr::for_each_type(my_preds_noexcept, []<class Pred> {
			do_test.template operator()<Pred>();
		});
	}
}

TEST_CASE("c_callable", "[u][engine][core][functional]") {
	CHECK(fr::c_callable<MyPred>);
	CHECK(fr::c_callable<decltype(&MyTrivial::foo)>);
	CHECK(fr::c_callable<int (float)>);
	CHECK(fr::c_callable<int (*)(float)>);

	CHECK_FALSE(fr::c_callable<int>);
	CHECK_FALSE(fr::c_callable<Incomplete>);
	CHECK_FALSE(fr::c_callable<MyTrivialLarge>);
}

namespace {

struct IntProperty {
	auto get() const -> int { return 10; }
	void set(int) { }

	static
	auto free_get() -> int { return 11;}
};

struct IntProperty2 {
	auto get() const -> int { return 20; }
	void set(int) { }
};

struct StringProperty {
	auto get() const -> const std::string& { return this->value; }
	void set(std::string new_value) { this->value = std::move(new_value); }

	auto get_int_prop() const -> IntProperty { return {}; }

public:
	std::string value;
};

} // namespace

TEST_CASE("c_getter_setter_pair", "[u][engine][core][functional]") {
	CHECK(fr::c_getter_setter_pair<&IntProperty::get, &IntProperty::set>);
	CHECK(fr::c_getter_setter_pair<&StringProperty::get, &StringProperty::set>);

	CHECK_FALSE(fr::c_getter_setter_pair<&IntProperty::get, &IntProperty::free_get>);
	CHECK_FALSE(fr::c_getter_setter_pair<&IntProperty::get, &IntProperty2::get>);
	CHECK_FALSE(fr::c_getter_setter_pair<&StringProperty::get, &StringProperty::get>);
	CHECK_FALSE(fr::c_getter_setter_pair<&IntProperty::get, &StringProperty::set>);
	CHECK_FALSE(fr::c_getter_setter_pair<&StringProperty::get_int_prop, &StringProperty::set>);
}

namespace {

auto free_func(int x) noexcept -> int {
	return x + 1;
}

auto call(fr::FunctionRef<int(int)> f, int param) -> int {
	return f(param);
}

struct Callable {
	auto operator()(int x) const { return x + this->b; }
	auto operator()(int x) { return x + this->b + 1; } // NOLINT

public:
	int b;
};

} // namespace

TEST_CASE("FunctionRef.basics", "[u][engine][core][functional]") {
	SECTION("free function") {
		CHECK(call(&free_func, 20) == 21);
	}
	SECTION("lambda") {
		CHECK(call([b = 2](int x) { return x + b; }, 20) == 22);
	}
	SECTION("immutable callable object") {
		const auto c = Callable{3};
		CHECK(call(c, 20) == 23);
	}
	SECTION("mutable callable object") {
		auto c = Callable{3};
		CHECK(call(c, 20) == 24);
	}
}
