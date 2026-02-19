#ifndef FRACTAL_BOX_CORE_CONTAINERS_UNORDERED_MAP_HPP
#define FRACTAL_BOX_CORE_CONTAINERS_UNORDERED_MAP_HPP

#include <unordered_map>

#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/hashing/uni_hasher.hpp"

namespace fr {

template<
	class K,
	class T,
	class H = UniHasherFastStd,
	class E = EqualTo<>,
	class A = std::allocator<std::pair<const K, T>>
>
using UnorderedMap = std::unordered_map<K, T, H, E, A>;

} // namespace fr
#endif
