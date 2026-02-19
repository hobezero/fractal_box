#ifndef FRACTAL_BOX_CORE_MEMORY_HPP
#define FRACTAL_BOX_CORE_MEMORY_HPP

#include <cstddef>

#include <bit>
#include <new>
#include <memory>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/int_types.hpp"

namespace fr {

template<class T>
inline constexpr
auto is_pow_of_two(T value) noexcept -> bool {
	return value > T{0} && ((value & (value - T{1})) == 0);
}

/* libc++ std::align implementation
void*
align(size_t alignment, size_t size, void*& ptr, size_t& space)
{
	void* r = nullptr;
	if (size <= space)
	{
		char* p1 = static_cast<char*>(ptr);
		char* p2 = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(p1 + (alignment - 1)) & -alignment);
		size_t d = static_cast<size_t>(p2 - p1);
		if (d <= space - size)
		{
			r = p2;
			ptr = r;
			space -= d;
		}
	}
	return r;
}
*/

inline
auto is_aligned_by(void* ptr, size_t alignment) noexcept -> bool {
	FR_ASSERT_AUDIT(is_pow_of_two(alignment));
	const auto addr = std::bit_cast<uintptr_t>(ptr);
	return (addr & (alignment - 1u)) == 0u;
}

/// @brief Calculate the byte offset needed to properly align a pointer
/// @pre `alignment` is a power of two
/// @see https://lesleylai.info/en/std-align/
inline
auto aligned_up_offset(void* ptr, size_t alignment) noexcept -> size_t {
	FR_ASSERT_AUDIT(is_pow_of_two(alignment));
	const auto addr_old = std::bit_cast<uintptr_t>(ptr);
	const auto addr_aligned = (addr_old + (alignment - 1zu)) & -alignment;
	return addr_aligned - addr_old;
}

/// @brief Calculate the byte offset needed to properly align a pointer
/// @pre `alignment` is a power of two
/// @see https://lesleylai.info/en/std-align/
inline constexpr
auto aligned_up_offset(uintptr_t address, size_t alignment) noexcept -> size_t {
	FR_ASSERT_AUDIT(is_pow_of_two(alignment));
	const auto addr_aligned = (address + (alignment - 1zu)) & -alignment;
	return addr_aligned - address;
}

/// @brief Calculate the first aligned address at or after the given pointer
/// @pre `alignment` is a power of two
inline
auto aligned_up(void* ptr, size_t alignment) noexcept -> void* {
	FR_ASSERT_AUDIT(is_pow_of_two(alignment));
	const auto addr_old = std::bit_cast<uintptr_t>(ptr);
	const auto addr_aligned = (addr_old + (alignment - 1zu)) & -alignment;
	return static_cast<void*>(static_cast<char*>(ptr) + (addr_aligned - addr_old));
}

class AlignedDeleter {
public:
	AlignedDeleter() = default;

	explicit
	AlignedDeleter(std::align_val_t alignment) noexcept:
		_alignment{alignment}
	{ }

	void operator()(std::byte* memory) const {
		::operator delete[](memory, _alignment);
	}

	auto alignment() const noexcept -> std::align_val_t { return _alignment; }

private:
	std::align_val_t _alignment = std::align_val_t{__STDCPP_DEFAULT_NEW_ALIGNMENT__};
};

using AlignedBuffer = std::unique_ptr<std::byte[], AlignedDeleter>;

inline
auto make_aligned_buffer(
	size_t size,
	std::align_val_t alignment = std::align_val_t{__STDCPP_DEFAULT_NEW_ALIGNMENT__}
) -> AlignedBuffer {
	FR_ASSERT(is_pow_of_two(static_cast<size_t>(alignment)));
	auto* const memory = static_cast<std::byte*>(::operator new[](size, alignment));
	return AlignedBuffer{memory, AlignedDeleter{alignment}};
}

} // namespace fr
#endif // include guard
