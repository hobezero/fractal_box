#include "fractal_box/core/type_index.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "detail/type_index_tu_helpers.hpp"
#include "fractal_box/core/algorithm.hpp"
#include "test_common/type_index_fmt.hpp"

namespace {

struct A { };
struct B { };
struct C { };
struct D { };
struct E { };

} // namespace

TEST_CASE("TypeIndex", "[u][engine][core][type_index]") {
	using DefaultValue = fr::DefaultTypeIndexDomain::ValueType;

	SECTION("type counter is just a constant") {
		CHECK(std::is_const_v<std::remove_reference_t<decltype(fr::type_index<int>)>>);
	}
	SECTION("type counter is of the type it claims to be") {
		CHECK(std::is_same_v<
			std::remove_cvref_t<decltype(fr::type_index<int>)>,
			fr::TypeIndex<>
		>);
	}
	// NOTE: Initialization of (inline) template variables is unordered according to the standard
	// (see [basic.start.dynamic])
	SECTION("count types") {
		CHECK(fr::test_all_unique_small(std::to_array({
			fr::type_index<int>,
			fr::type_index<A>,
			fr::type_index<B>,
			fr::type_index<D>,
		})));
		SECTION("different domain means an independent sequence") {
			// To properly test the type sequence, make sure that the domain types are
			// inaccessible by anyone else
			struct DomainX: fr::DefaultTypeIndexDomain { };
			struct DomainY: fr::DefaultTypeIndexDomain { };
			using TypeIdxX = fr::TypeIndex<DomainX>;
			using TypeIdxY = fr::TypeIndex<DomainY>;

			// NOTE: It is safe to assume that the type index of B is zero since there are no other
			// template instantiations (note the unique domain type)
			CHECK(fr::type_index<B, DomainX> == TypeIdxX{fr::adopt, 0});

			const TypeIdxY::ValueType y_list[] = {
				TypeIdxY::of<C>.value(),
				TypeIdxY::of<B>.value(),
				TypeIdxY::of<E>.value()
			};

			// We don't care about the exact order as long as there are no collisions and skipped
			// values
			CHECK(std::ranges::is_permutation(y_list,
				std::views::iota(TypeIdxY::ValueType{0}, std::size(y_list))));

			CHECK(fr::test_all_unique_small(std::to_array({
				std::bit_cast<uintptr_t>(&fr::type_index<B>),
				std::bit_cast<uintptr_t>(&fr::type_index<B, DomainX>),
				std::bit_cast<uintptr_t>(&fr::type_index<B, DomainY>),
			})));
		}
	}
	SECTION("type sequence is consistent across multiple TUs") {
		namespace ti = frt::detail_type_index;
		using TypeIdxDummy = fr::TypeIndex<ti::DummyDomain>;
		// NOTE: The instantiation order is different here
		CHECK(TypeIdxDummy::of<ti::Dummy2> == ti::dummy_type_idx(2));
		CHECK(TypeIdxDummy::of<ti::Dummy3> == ti::dummy_type_idx(3));
		CHECK(TypeIdxDummy::of<ti::Dummy0> == ti::dummy_type_idx(0));
		CHECK(TypeIdxDummy::of<ti::Dummy1> == ti::dummy_type_idx(1));
	}
	SECTION("type sequence starting from a non-zero value is correct") {
		static constexpr auto starting_value = DefaultValue{4000};
		struct DomainP: fr::CustomTypeIndexDomainBase<DefaultValue, starting_value> { };
		using TypeIdxP = fr::TypeIndex<DomainP>;
		const DefaultValue p_list[] = {
			TypeIdxP::of<A>.value(),
			TypeIdxP::of<C>.value(),
			TypeIdxP::of<D>.value(),
			TypeIdxP::of<B>.value(),
			TypeIdxP::of<E>.value(),
		};

		CHECK(std::ranges::is_permutation(p_list,
			std::views::iota(DefaultValue{starting_value}) | std::views::take(std::size(p_list))));
	}
	SECTION("comparison") {
		CHECK(fr::type_index<int> == fr::type_index<int>);
		CHECK(fr::type_index<int> != fr::type_index<char>);
	}
}
