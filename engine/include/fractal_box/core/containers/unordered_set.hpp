#ifndef FRACTAL_BOX_CORE_CONTAINERS_UNORDERED_SET_HPP
#define FRACTAL_BOX_CORE_CONTAINERS_UNORDERED_SET_HPP

#include <unordered_set>

#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/hashing/uni_hasher.hpp"

namespace fr {

template<class T, class H = UniHasherFastStd, class E = EqualTo<>, class A = std::allocator<T>>
using UnorderedSet = std::unordered_set<T, H, E, A>;

} // namespace fr
#endif
