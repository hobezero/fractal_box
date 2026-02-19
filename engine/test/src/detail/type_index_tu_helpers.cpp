#include "detail/type_index_tu_helpers.hpp"

namespace frt::detail_type_index {

auto dummy_type_idx(int dummy_idx) -> TypeIdxDummy {
	switch (dummy_idx) {
		case 0: return TypeIdxDummy::of<Dummy0>;
		case 1: return TypeIdxDummy::of<Dummy1>;
		case 2: return TypeIdxDummy::of<Dummy2>;
		case 3: return TypeIdxDummy::of<Dummy3>;
	}
	return {fr::adopt, static_cast<TypeIdxDummy::ValueType>(-1)};
}

} // namespace frt::detail_type_index
