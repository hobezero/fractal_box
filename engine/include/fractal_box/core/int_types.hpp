#ifndef FRACTAL_BOX_CORE_INT_TYPES_HPP
#define FRACTAL_BOX_CORE_INT_TYPES_HPP

#include <cstddef>
#include <cstdint>

namespace fr {

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

using std::uint_fast8_t;
using std::uint_fast16_t;
using std::uint_fast32_t;
using std::uint_fast64_t;

using std::int_fast8_t;
using std::int_fast16_t;
using std::int_fast32_t;
using std::int_fast64_t;

using std::uint_least8_t;
using std::uint_least16_t;
using std::uint_least32_t;
using std::uint_least64_t;

using std::int_least8_t;
using std::int_least16_t;
using std::int_least32_t;
using std::int_least64_t;

using std::size_t;
using std::ptrdiff_t;
using std::uintptr_t;
using std::intptr_t;

inline constexpr auto npos = static_cast<size_t>(-1);

template<class T>
inline constexpr auto npos_for = static_cast<T>(-1);

} // namespace fr
#endif // include guard
