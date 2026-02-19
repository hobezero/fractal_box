#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/poly.hpp"

namespace {

struct A {
	FR_POLY_DEFINE_ROOT()
	virtual ~A() = default;
};

struct E : A { FR_POLY_DEFINE_ID(::E, A) };
struct B : A { FR_POLY_DEFINE_ID(::B, A) };
struct C : B { FR_POLY_DEFINE_ID(::C, B) };
struct D : B { FR_POLY_DEFINE_ID(::D, B) };
struct F : C { FR_POLY_DEFINE_ID(::F, C) };

struct X {
	FR_POLY_DEFINE_ROOT()
	virtual ~X() = default;
};

struct Y : X { FR_POLY_DEFINE_ID(::Y, X) };

} // namespace

TEST_CASE("poly_is", "[u][engine][core][poly]") {
	A a; E e; B b; C c; D d; F f;
	X x; Y y;

	SECTION("upcast is trivial") {
		CHECK(fr::poly::is<A>(a));
		CHECK(fr::poly::is<A>(static_cast<A&>(c)));
		CHECK(fr::poly::is<A>(c));
		CHECK(fr::poly::is<B>(c));
		CHECK(fr::poly::is<B>(f));
		CHECK(fr::poly::is<A>(f));
		CHECK(fr::poly::is<X>(y));
	}

	SECTION("downcast") {
		CHECK(fr::poly::is<B>(static_cast<A&>(b)));
		CHECK(fr::poly::is<B>(static_cast<A&>(c)));
		CHECK(fr::poly::is<B>(static_cast<A&>(f)));
		CHECK(fr::poly::is<C>(static_cast<B&>(f)));
		CHECK(fr::poly::is<C>(static_cast<B&>(f)));
		CHECK(fr::poly::is<F>(static_cast<B&>(f)));
	}

	SECTION("sidecast") {
		CHECK(!fr::poly::is<D>(static_cast<A&>(e)));
		CHECK(!fr::poly::is<C>(static_cast<B&>(d)));
		CHECK(!fr::poly::is<F>(static_cast<B&>(d)));
	}
}

TEST_CASE("dynCast", "[u][engine][core][poly]") {
	A a; E e; B b; C c; D d; F f;
	X x; Y y;

	SECTION("upcast is trivial") {
		CHECK(fr::poly::dynCast<A*>(&a));
		CHECK(fr::poly::dynCast<A*>(static_cast<A*>(&c)));
		CHECK(fr::poly::dynCast<A*>(&c));
		CHECK(fr::poly::dynCast<B*>(&c));
		CHECK(fr::poly::dynCast<B*>(&f));
		CHECK(fr::poly::dynCast<A*>(&f));
		CHECK(fr::poly::dynCast<X*>(&y));
	}

	SECTION("downcast") {
		CHECK(fr::poly::dynCast<B*>(static_cast<A*>(&b)));
		CHECK(fr::poly::dynCast<B*>(static_cast<A*>(&c)));
		CHECK(fr::poly::dynCast<B*>(static_cast<A*>(&f)));
		CHECK(fr::poly::dynCast<C*>(static_cast<B*>(&f)));
		CHECK(fr::poly::dynCast<C*>(static_cast<B*>(&f)));
		CHECK(fr::poly::dynCast<F*>(static_cast<B*>(&f)));
	}

	SECTION("sidecast") {
		CHECK(!fr::poly::dynCast<D*>(static_cast<A*>(&e)));
		CHECK(!fr::poly::dynCast<C*>(static_cast<B*>(&d)));
		CHECK(!fr::poly::dynCast<F*>(static_cast<B*>(&d)));
	}
}
