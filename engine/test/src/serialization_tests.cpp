#include "fractal_box/core/serialization/serialization_concepts.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

struct CustomFriend {
	template<class Ar>
	friend constexpr
	void fr_custom_serialize(Ar& archive, fr::AddConstIf<CustomFriend, Ar::is_write>& self) {
		archive(self.x, self.y);
	}

public:
	int x;
	std::string y;
};

struct CustomStatic {
	static constexpr
	void fr_custom_serialize(auto& archive, auto& self) {
		archive(self.x, self.y);
	}

public:
	int x;
	std::string y;
};

} // namespace

TEST_CASE("Serialization-concepts", "[u][engine][core][serialization]") {
	STATIC_CHECK(fr::c_has_custom_serialize<CustomFriend>);
	STATIC_CHECK(fr::c_has_custom_serialize<CustomStatic>);
}
