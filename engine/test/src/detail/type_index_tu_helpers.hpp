#ifndef FR_TEST_DETAIL_TYPE_INDEX_TU_HELPERS_HPP
#define FR_TEST_DETAIL_TYPE_INDEX_TU_HELPERS_HPP

#include "fractal_box/core/type_index.hpp"

// Make absolutely sure there are no name collisions
namespace frt::detail_type_index {

using Underlying = unsigned;

struct DummyDomain: fr::DefaultTypeIndexDomain { };
using TypeIdxDummy = fr::TypeIndex<DummyDomain>;

struct Dummy0 { };
struct Dummy1 { };
struct Dummy2 { };
struct Dummy3 { };

auto dummy_type_idx(int dummy_id) -> TypeIdxDummy;

} // namespace frt::detail_type_index
#endif
