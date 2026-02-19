#ifndef FR_TEST_TEST_COMMON_TYPE_INDEX_FMT_HPP
#define FR_TEST_TEST_COMMON_TYPE_INDEX_FMT_HPP

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/type_index.hpp"

template<class Domain>
struct Catch::StringMaker<fr::TypeIndex<Domain>> {
	static
	auto convert(const fr::TypeIndex<Domain>& type_idx) -> std::string {
		using V = typename Domain::ValueType;
		return Catch::StringMaker<V>::convert(type_idx.value());
	}
};

#endif // include guard
