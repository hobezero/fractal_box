#ifndef FRACTAL_BOX_CORE_SERIALIZATION_SERIALIZATION_CONCEPTS_HPP
#define FRACTAL_BOX_CORE_SERIALIZATION_SERIALIZATION_CONCEPTS_HPP

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/error_handling/result.hpp"
#include "fractal_box/core/serialization/serialization_attributes.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/meta/reflection.hpp"

namespace fr {

namespace detail {

struct DummyEncodingArchive {
	static constexpr auto is_encoding = true;
	static constexpr auto is_decoding = false;

	constexpr
	auto operator()(const auto&...) noexcept { }
};

struct DummyDecodingArchive {
	static constexpr auto is_encoding = false;
	static constexpr auto is_decoding = true;

	constexpr
	auto operator()(auto&...) noexcept { }
};

struct DummyWriter {
	using CharType = std::byte;
	static constexpr auto is_buffered = false;
	static constexpr auto is_buffer_resizable = false;

	constexpr
	auto write(std::span<const std::byte>) noexcept -> size_t { return 0zu; }

	constexpr
	void flush() noexcept { }

};

struct DummyReader {
	using CharType = std::byte;
	static constexpr auto is_buffered = false;

	constexpr
	auto read(std::span<std::byte>) noexcept -> size_t { return 0zu; }

	constexpr
	void read_exact(std::span<std::byte>) noexcept { }
};

} // namespace detail

template<class T>
concept c_data_format = requires(
	detail::DummyWriter& writer,
	detail::DummyReader& reader,
	int obj
) {
	typename T::template EncodingArchive<detail::DummyWriter>;
	typename T::template DecodingArchive<detail::DummyReader>;

	typename T::template EncodeResult<detail::DummyWriter>;
	typename T::template DecodeResult<detail::DummyReader>;

	{ T::encode(writer, obj) } -> std::same_as<
		typename T::template EncodeResult<detail::DummyWriter>>;
	{ T::decode(reader, obj) } -> std::same_as<
		typename T::template DecodeResult<detail::DummyReader>>;
};

template<class T>
concept c_has_custom_serialize = requires(
	const T& const_obj,
	T& mut_obj,
	detail::DummyEncodingArchive& enc_archive,
	detail::DummyDecodingArchive& dec_archive
) {
	requires requires { { fr_custom_serialize(enc_archive, const_obj) } -> c_void_or_result; }
		|| requires { { T::fr_custom_serialize(enc_archive, const_obj) } -> c_void_or_result; };
	requires requires { { fr_custom_serialize(dec_archive, mut_obj) } -> c_void_or_result; }
		|| requires { { T::fr_custom_serialize(dec_archive, mut_obj) } -> c_void_or_result; };
};

template<class T, class DataFormat>
concept c_serializable_by = requires(
	detail::DummyWriter& writer,
	detail::DummyReader& reader,
	const T& const_obj,
	T& mut_obj
) {
	{ DataFormat::encode(writer, const_obj) } -> c_void_or_result;
	{ DataFormat::decode(reader, mut_obj) } -> c_void_or_result;
};

enum class SerializableCategory: uint8_t {
	Unserializable,
	Primitive,
	Custom,
	Described,
	Enum,
	Optional,
	String,
	Array,
	Vector,
	Map,
	Set,
	Variant,
	Record,
};

class Serializability {
public:
	Serializability() = default;

	FR_FORCE_INLINE constexpr
	Serializability(SerializableCategory category, SerializableMode mode) noexcept:
		_category{category},
		_mode{mode}
	{
		if (_category == SerializableCategory::Unserializable) {
			FR_ASSERT_MSG(_mode == SerializableMode::None, "Category/mode mismatch");
		}
	}

	friend
	auto operator==(Serializability, Serializability) -> bool = default;

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept {
		return _mode != SerializableMode::None;
	}

	FR_FORCE_INLINE constexpr
	auto category() const noexcept -> SerializableCategory { return _category; }

	FR_FORCE_INLINE constexpr
	auto mode() const noexcept -> SerializableMode { return _mode; }

private:
	SerializableCategory _category = SerializableCategory::Unserializable;
	SerializableMode _mode = SerializableMode::None;
};

template<class T>
inline consteval
auto get_serializability() noexcept -> Serializability;

namespace detail {

template<SerializableMode Mode, class Child>
inline consteval
void verify_serializable_child() noexcept {

}

} // namespace detail

template<class T>
using IsSerializable = BoolC<bool{get_serializability<T>()}>;

template<class T>
inline consteval
auto get_serializability() noexcept -> Serializability {
	using PT = std::remove_cvref_t<T>;

	using enum SerializableMode;
	using enum SerializableCategory;

	if constexpr (std::is_fundamental_v<PT>) {
		static_assert(!c_has_custom_serialize<PT>, "Can't customize serialization for primitives");
		if constexpr (std::is_arithmetic_v<PT>) {
			return {Primitive, Default};
		}
		else {
			return {Primitive, None};
		}
	}
	else if constexpr (c_has_custom_serialize<PT>) {
		return {Custom, Default};
	}
	else if constexpr (c_has_describe<PT>) {
		static_assert(!refl_has_attribute<PT, SerializableCategory>,
			"SerializableCategory is not an attribute");
		if constexpr (refl_has_attribute<PT, SerializableMode>) {
			static_assert(!refl_has_attribute<PT, Serializable>,
				"Conflicting SerializableMode and Serializable attributes");
			static constexpr auto mode = refl_attribute<PT, SerializableMode>;
			if constexpr (mode == OptOut) {
				static_assert(mp_all_of<ReflBases<PT>, IsSerializable>,
					"Serializable class must have serializable bases");
			}
			[]<class... FPs>(MpList<FPs...>) {
				(..., detail::verify_serializable_child<mode, FPs>());
			}(ReflFieldsAndProperties<PT>{});
			return {Described, mode};
		}
		else if constexpr (refl_has_attribute<PT, Serializable>) {
			static constexpr auto mode = refl_attribute<PT, Serializable>.value ? OptOut : None;
			if constexpr (mode == OptOut) {
				static_assert(mp_all_of<ReflBases<PT>, IsSerializable>,
					"Serializable class must have serializable bases");
			}
			[]<class... FPs>(MpList<FPs...>) {
				(..., detail::verify_serializable_child<mode, FPs>());
			}(ReflFieldsAndProperties<PT>{});
			return {Described, mode};
		}
		else {
			return {Described, None};
		}
	}
	else if constexpr (std::is_enum_v<PT>) {
		return {Enum, get_serializability<std::underlying_type_t<PT>>().mode()};
	}
	else if constexpr (c_optional_like<PT>) {
		return {Optional, get_serializability<typename PT::value_type>() ? Default : None};
	}
	else if constexpr (c_string_like<PT>) {
		return {String, Default};
	}
	else if constexpr (std::is_bounded_array_v<PT>) {
		return {Array, get_serializability<std::remove_all_extents<PT>>().mode()};
	}
	else if constexpr (c_std_array_like<PT>) {
		return {Array, get_serializability<typename PT::value_type>().mode()};
	}
	else if constexpr (c_vector_like<PT>) {
		return {Vector, get_serializability<typename PT::value_type>().mode()};
	}
	// TODO: Map, Set, Variant
	else if constexpr (c_record_like<PT>) {
		using Decomposed = ReflDecomposition<PT>;
		if constexpr (mp_all_of<Decomposed, IsSerializable>) {
			return {Record, Default};
		}
		else {
			return {Record, None};
		}
	}
	else {
		return {Unserializable, None};
	}
}

template<class T>
concept c_serializable = bool{get_serializability<std::remove_cvref_t<T>>()};

} // namespace fr
#endif // include guard
