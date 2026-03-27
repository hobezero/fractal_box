#ifndef FRACTAL_BOX_CORE_META_TYPE_INFO_HPP
#define FRACTAL_BOX_CORE_META_TYPE_INFO_HPP

#include "fractal_box/core/hashing/hashed_string.hpp"
#include "fractal_box/core/meta/reflection.hpp"

namespace fr {

class alignas(hardware_constructive_interference_size) TypeInfo {
	using LengthType = uint16_t;

public:
	template<class T>
	explicit constexpr
	TypeInfo(InPlaceAsInit<T>) noexcept:
		_custom_name_hash{StringId::calc_hash(refl_custom_name<T>.data(),
			refl_custom_name<T>.size())},
		_hash32{type_hash32<T>},
		_hash64{type_hash64<T>},
		_name{type_name_lit<T>.data()},
		_custom_name{refl_custom_name<T>.data()},
		_display_name{refl_display_name<T>.data()},
		// TODO: Replace `static_cast`s with `narrow_cast`s
		_name_length{static_cast<LengthType>(type_name_lit<T>.size())},
		_custom_name_length{static_cast<LengthType>(refl_custom_name<T>.size())},
		_display_name_length{static_cast<LengthType>(refl_display_name<T>.size())},
		_alignment{static_cast<decltype(_alignment)>(alignof(T))},
		_size{static_cast<decltype(_size)>(sizeof(T))}
	{ }

	constexpr
	auto operator==(const TypeInfo& other) const noexcept -> bool {
		return _hash32 == other._hash32
			&& _hash64 == other._hash64;
	}

	constexpr
	auto hash32() const noexcept -> HashDigest32 { return _hash32; }

	constexpr
	auto hash64() const noexcept -> HashDigest64 { return _hash64; }

	constexpr
	auto name() const noexcept -> std::string_view {
		return std::string_view(_name, static_cast<size_t>(_name_length));
	}

	constexpr
	auto name_id() const noexcept -> StringId { return StringId{adopt, _hash32}; }

	constexpr
	auto hashed_name() const noexcept -> HashedStrView {
		return HashedStrView{adopt, _hash32, name()};
	}

	constexpr
	auto custom_name() const noexcept -> std::string_view {
		return std::string_view(_custom_name, static_cast<size_t>(_custom_name_length));
	}

	constexpr
	auto custom_name_id() const noexcept -> StringId { return StringId{adopt, _custom_name_hash}; }

	constexpr
	auto hashed_custom_name() const noexcept -> HashedStrView {
		return HashedStrView{adopt, _custom_name_hash, custom_name()};
	}

	constexpr
	auto display_name() const noexcept -> std::string_view {
		return std::string_view(_display_name, static_cast<size_t>(_display_name_length));
	}

	constexpr
	auto alignment() const noexcept -> size_t { return static_cast<size_t>(_alignment); }

	constexpr
	auto size() const noexcept -> size_t { return static_cast<size_t>(_size); }

private:
	HashDigest32 _custom_name_hash;
	HashDigest32 _hash32;
	HashDigest64 _hash64;
	const char* _name;
	const char* _custom_name;
	const char* _display_name;
	LengthType _name_length;
	LengthType _custom_name_length;
	LengthType _display_name_length;
	uint16_t _alignment;
	uint32_t _size;
};

template<class T>
inline constexpr auto type_info = TypeInfo{in_place_as<T>};

} // namespace fr
#endif // include guard
