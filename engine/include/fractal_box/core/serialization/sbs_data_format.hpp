#ifndef FRACTAL_BOX_CORE_SERIALIZATION_SBS_DATA_FORMAT_HPP
#define FRACTAL_BOX_CORE_SERIALIZATION_SBS_DATA_FORMAT_HPP

#include <cstring>

#include <utility>

#include "fractal_box/core/byte_utils.hpp"
#include "fractal_box/core/containers/simple_array.hpp"
#include "fractal_box/core/io/io_concepts.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/serialization/serialization_concepts.hpp"

namespace fr {

template<class V>
inline constexpr
void set_variant_valueless(V& variant) {
	try {
		variant.template emplace<0zu>(detail::ThrowingConvertible<MpFirst<V>>{});
	}
	catch (int) { }
}

template<class V>
inline constexpr
auto make_valueless_variant() -> V {
	auto v = V{};
	set_variant_valueless(v);
	return v;
}

/// @brief Simple binary serialization format
struct SbsDataFormat {
private:
	static_assert(std::endian::native == std::endian::little);

	template<class Writer>
	static consteval
	auto calc_encode_result_type() noexcept {
		using Char = typename Writer::CharType;
		using WriteResult = decltype(std::declval<Writer&>().write(
			std::declval<std::span<const Char>>()));
		return mp_type<WriteResult>;
	}

	template<class Reader>
	static consteval
	auto calc_decode_result_type() noexcept {
		using Char = typename Reader::CharType;
		using ReadResult = decltype(std::declval<Reader&>().read(std::declval<std::span<Char>>()));
		return mp_type<ReadResult>;
	}

	template<std::unsigned_integral T>
	static constexpr auto max_value = static_cast<T>(-1);

	template<size_t NumAlternatives>
	static consteval
	auto calc_variant_index_type() noexcept {
		// Reserve the max value as a valueless indicator
		if constexpr (NumAlternatives < static_cast<size_t>(max_value<uint8_t>)) {
			return uint8_t{};
		}
		else if constexpr (NumAlternatives < static_cast<size_t>(max_value<uint16_t>)) {
			return uint16_t{};
		}
		else if constexpr (NumAlternatives < static_cast<size_t>(max_value<uint32_t>)) {
			return uint32_t{};
		}
		else {
			// I don't think any compiler can handle a template with 4.2 billion types
			return uint64_t{};
		}
	}
public:
	template<c_byte_writer Writer>
	class EncodingArchive;

	template<c_byte_reader Reader>
	class DecodingArchive;

	template<c_byte_writer Writer>
	using EncodeResult = typename decltype(calc_encode_result_type<Writer>())::Type;

	template<c_byte_reader Reader>
	using DecodeResult = typename decltype(calc_decode_result_type<Reader>())::Type;

	template<class Variant>
	using VariantIndexType = decltype(calc_variant_index_type<mp_size<Variant>>());

	template<c_byte_writer Writer, c_serializable T>
	static FR_FORCE_INLINE constexpr
	auto encode(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		using enum SerializableCategory;
		static constexpr auto serializability = get_serializability<T>();
		static_assert(serializability);
		if constexpr (serializability.category() == Primitive) {
			return encode_primitive(writer, obj);
		}
		else if constexpr (serializability.category() == Custom) {
			auto archive = EncodingArchive<Writer>{writer};
			if constexpr (requires { T::fr_custom_serialize(archive, obj); }) {
				return T::fr_custom_serialize(archive, obj);
			}
			else {
				return fr_custom_serialize(archive, obj);
			}
		}
		else if constexpr (serializability.category() == Described) {
			static_assert(false);
		}
		else if constexpr (serializability.category() == Enum) {
			return encode_enum(writer, obj);
		}
		else if constexpr (serializability.category() == Optional) {
			return encode_optional(writer, obj);
		}
		else if constexpr (serializability.category() == String) {
			return encode_string(writer, obj);
		}
		else if constexpr (serializability.category() == Array) {
			return encode_array(writer, obj);
		}
		else if constexpr (serializability.category() == Vector) {
			return encode_vector(writer, obj);
		}
		else if constexpr (serializability.category() == Map) {
			static_assert(false);
		}
		else if constexpr (serializability.category() == Set) {
			return encode_set(writer, obj);
		}
		else if constexpr (serializability.category() == Variant) {
			return encode_variant(writer, obj);
		}
		else if constexpr (serializability.category() == Record) {
			static_assert(false);
		}
		else {
			static_assert(false);
		}
	}

	template<c_byte_reader Reader, c_serializable T>
	static FR_FORCE_INLINE constexpr
	auto decode(Reader& reader, T& obj) -> DecodeResult<Reader> {
		using enum SerializableCategory;
		static constexpr auto serializability = get_serializability<T>();
		static_assert(serializability);
		if constexpr (serializability.category() == Primitive) {
			return decode_primitive(reader, obj);
		}
		else if constexpr (serializability.category() == Custom) {
			auto archive = DecodingArchive<Reader>{reader};
			if constexpr (requires { T::fr_custom_serialize(archive, obj); }) {
				return T::fr_custom_serialize(archive, obj);
			}
			else {
				return fr_custom_serialize(archive, obj);
			}
		}
		else if constexpr (serializability.category() == Described) {
			static_assert(false);
		}
		else if constexpr (serializability.category() == Enum) {
			return decode_enum(reader, obj);
		}
		else if constexpr (serializability.category() == Optional) {
			return decode_optional(reader, obj);
		}
		else if constexpr (serializability.category() == String) {
			return decode_string(reader, obj);
		}
		else if constexpr (serializability.category() == Array) {
			return decode_array(reader, obj);
		}
		else if constexpr (serializability.category() == Vector) {
			return decode_vector(reader, obj);
		}
		else if constexpr (serializability.category() == Map) {
			static_assert(false);
		}
		else if constexpr (serializability.category() == Set) {
			return decode_set(reader, obj);
		}
		else if constexpr (serializability.category() == Variant) {
			return decode_variant(reader, obj);
		}
		else if constexpr (serializability.category() == Record) {
			static_assert(false);
		}
		else {
			static_assert(false);
		}
	}

private:
	template<class T>
	static FR_FORCE_INLINE constexpr
	auto result_size(const T& result) noexcept -> size_t {
		if constexpr (is_result_of<T, size_t>) {
			if (!result)
				return 0zu;
			return result.value();
		}
		else if constexpr (std::is_same_v<T, size_t>) {
			return result;
		}
		else {
			static_assert(false);
		}
	}

	template<class T>
	static FR_FORCE_INLINE constexpr
	void set_result_size(T& result, size_t new_size) noexcept {
		if constexpr (is_result_of<T, size_t>) {
			if (result)
				result.value() = new_size;
		}
		else if constexpr (std::is_same_v<T, size_t>) {
			result = new_size;
		}
		else {
			static_assert(false);
		}
	}

	// Primitives
	// ^^^^^^^^^^

	template<class Writer, class T>
	static constexpr
	auto encode_primitive(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		using Char = typename Writer::CharType;
		if constexpr (std::is_arithmetic_v<T>) {
			if consteval {
				const auto obj_bytes = std::bit_cast<SimpleArray<Char, sizeof(T)>>(obj);
				return writer.write(std::span<const Char>(obj_bytes.data(), sizeof(T)));
			}
			else {
				return writer.write(std::span<const Char>(
					reinterpret_cast<const Char*>(std::addressof(obj)),
					sizeof(T)
				));
			}
		}
		else {
			static_assert(false);
		}
	}

	template<class Reader, class T>
	static constexpr
	auto decode_primitive(Reader& reader, T& obj) -> DecodeResult<Reader> {
		using Char = typename Reader::CharType;
		if constexpr (std::is_arithmetic_v<T>) {
			if consteval {
				auto obj_bytes = SimpleArray<Char, sizeof(T)>{};
				if constexpr (c_result<DecodeResult<Reader>>) {
					auto res = reader.read_exact(std::span<Char>(obj_bytes.data(), sizeof(T)));
					if (!res)
						return {from_error, std::move(res)};
					obj = std::bit_cast<T>(obj_bytes);
					return {in_place, sizeof(T)};
				}
				else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
					reader.read_exact(std::span<Char>(obj_bytes.data(), sizeof(T)));
					obj = std::bit_cast<T>(obj_bytes);
					return sizeof(T);
				}
				else {
					static_assert(false);
				}
			}
			else {
				if constexpr (c_result<DecodeResult<Reader>>) {
					auto res = reader.read_exact(std::span<Char>(reinterpret_cast<Char*>(&obj),
						sizeof(T)));
					if (!res)
						return {from_error, std::move(res)};
					return {in_place, sizeof(T)};
				}
				else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
					reader.read_exact(std::span<Char>(reinterpret_cast<Char*>(&obj), sizeof(T)));
					return sizeof(T);
				}
				else {
					static_assert(false);
				}
			}
		}
		else {
			static_assert(false);
		}
	}

	// Enums
	// ^^^^^

	template<class Writer, class E>
	static FR_FORCE_INLINE constexpr
	auto encode_enum(Writer& writer, E obj) -> EncodeResult<Writer> {
		return encode_primitive(writer, std::to_underlying(obj));
	}

	template<class Reader, class E>
	static FR_FORCE_INLINE constexpr
	auto decode_enum(Reader& reader, E& obj) -> DecodeResult<Reader> {
		std::underlying_type_t<E> value;
		auto result = decode_primitive(reader, value);
		obj = static_cast<E>(value);
		return result;
	}

	template<class Writer, class T>
	static constexpr
	auto encode_optional(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		auto has_value = bool{obj.has_value()};
		auto ret = encode_primitive(writer, has_value);

		if constexpr (c_result<EncodeResult<Writer>>) {
			if (!ret)
				return ret;
			if (has_value) {
				auto res = SbsDataFormat::encode(writer, *obj);
				if (res)
					*ret += *res;
				else
					return res;
			}
		}
		else if constexpr (std::is_same_v<EncodeResult<Writer>, size_t>){
			if (has_value)
				ret += SbsDataFormat::encode(writer, *obj);
		}
		else {
			static_assert(false);
		}
		return ret;
	}

	template<class Reader, class T>
	static constexpr
	auto decode_optional(Reader& reader, T& obj) -> DecodeResult<Reader> {
		bool has_value;
		auto ret = decode_primitive(reader, has_value);

		if constexpr (c_result<DecodeResult<Reader>>) {
			if (!ret)
				return ret;
			if (has_value) {
				if (!obj.has_value())
					obj.emplace();
				auto res = SbsDataFormat::decode(reader, *obj);
				if (!res)
					return res;
				*ret += *res;
			}
			else if (obj.has_value()) {
				obj.reset();
			}
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
			if (has_value) {
				if (!obj.has_value())
					obj.emplace();
				ret += SbsDataFormat::decode(reader, *obj);
			}
			else if (obj.has_value()) {
				obj.reset();
			}
		}
		else {
			static_assert(false);
		}
		return ret;
	}

	// Strings
	// ^^^^^^^

	template<class Writer, class T>
	static constexpr
	auto encode_string(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		auto size_res = encode_primitive(writer, static_cast<size_t>(obj.size()));
		if (!size_res)
			return size_res;

		using WriterChar = typename Writer::CharType;
		using StringChar = typename T::value_type;
		if constexpr (std::is_same_v<WriterChar, StringChar>) {
			auto data_res = writer.write(std::span<const WriterChar>(obj.data(), obj.size()));
			set_result_size(data_res, result_size(size_res) + result_size(data_res));
			return data_res;
		}
		else {
			const auto bytes_size = sizeof(StringChar) * obj.size();
			if consteval {
				auto* bytes = new WriterChar[bytes_size];
				write_str_as_bytes(bytes, obj.data(), obj.size());
				auto data_res = writer.write(std::span<const WriterChar>(bytes, bytes_size));
				delete[] bytes;
				set_result_size(data_res, result_size(size_res) + result_size(data_res));
				return data_res;
			}
			else {
				auto data_res = writer.write(std::span<const WriterChar>(
					reinterpret_cast<const WriterChar*>(obj.data()),
					bytes_size
				));
				set_result_size(data_res, result_size(size_res) + result_size(data_res));
				return data_res;
			}
		}
	}

	template<class Reader, class T>
	static constexpr
	auto decode_string(Reader& reader, T& obj) -> DecodeResult<Reader> {
		size_t size_value;
		auto size_res = decode_primitive(reader, size_value);
		if (!size_res)
			return size_res;

		using ReaderChar = typename Reader::CharType;
		using StringChar = typename T::value_type;

		const auto old_size = obj.size();
		const auto bytes_size = sizeof(StringChar) * size_value;
		if constexpr (c_result<DecodeResult<Reader>>) {
			using ReadExactResult = typename DecodeResult<Reader>::template Rebind<void>;
			ObjectStorage<ReadExactResult> data_res;

			if constexpr (std::is_same_v<ReaderChar, StringChar>) {
				obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
					std::construct_at(data_res.ptr(),
						reader.read_exact(std::span<ReaderChar>(data, n)));
					return n;
				});
			}
			else {
				if consteval {
					obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
						auto* bytes = new ReaderChar[bytes_size];
						std::construct_at(data_res.ptr(),
							reader.read_exact(std::span<ReaderChar>(bytes, bytes_size)));
						if (data_res.value)
							read_str_from_bytes(data, n, bytes);
						delete[] bytes;
						return n;
					});
				}
				else {
					obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
						std::construct_at(data_res.ptr(),
							reader.read_exact(std::span<ReaderChar>(
								reinterpret_cast<ReaderChar*>(data), bytes_size)));
						return n;
					});
				}
			}

			if (!data_res.value) {
				obj.resize(old_size);
				return {from_error, std::move(data_res).value};
			}
			return {in_place, result_size(size_res) + bytes_size};
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
			// Same thing, except read_exact returns void
			if constexpr (std::is_same_v<ReaderChar, StringChar>) {
				obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
					reader.read_exact(std::span<ReaderChar>(data, n));
					return n;
				});
			}
			else {
				if consteval {
					obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
						auto* bytes = new ReaderChar[bytes_size];
						reader.read_exact(std::span<ReaderChar>(bytes, bytes_size));
						read_str_from_bytes(data, n, bytes);
						delete[] bytes;
						return n;
					});
				}
				else {
					obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
						reader.read_exact(std::span<ReaderChar>(reinterpret_cast<ReaderChar*>(data),
							bytes_size));
						return n;
					});
				}
			}

			return result_size(size_res) + bytes_size;
		}
		else {
			static_assert(false);
		}
	}

	template<class Writer, class T>
	static constexpr
	auto encode_array(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		if constexpr (std::size(obj) == 0zu) {
			// std::array can have zero size
			return 0zu;
		}
		else if constexpr (c_result<EncodeResult<Writer>>) {
			auto ret = SbsDataFormat::encode(writer, obj[0zu]);
			if (!ret)
				return ret;
			for (auto i = 1zu; i < std::size(obj); ++i) {
				auto res = SbsDataFormat::encode(writer, obj[i]);
				if (res ) {
					*ret += *res;
				}
				else {
					ret = std::move(res);
					break;
				}
			}
			return ret;
		}
		else if constexpr (std::is_same_v<EncodeResult<Writer>, size_t>) {
			auto ret = 0zu;
			for (const auto& v : obj) {
				ret += SbsDataFormat::encode(writer, v);
			}
			return ret;
		}
		else {
			static_assert(false);
		}
	}

	template<class Reader, class T>
	static constexpr
	auto decode_array(Reader& reader, T& obj) -> DecodeResult<Reader> {
		static constexpr auto size = std::size(obj);
		if constexpr (size == 0zu) {
			// std::array can have zero size
			return 0zu;
		}
		else if constexpr (c_result<DecodeResult<Reader>>) {
			auto ret = SbsDataFormat::decode(reader, obj[0zu]);
			if (!ret)
				return ret;
			for (auto i = 1zu; i < size; ++i) {
				auto res = SbsDataFormat::decode(reader, obj[i]);
				if (!res)
					return res;
				*ret += *res;
			}
			return ret;
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
			auto ret = 0zu;
			for (auto& v : obj) {
				ret += SbsDataFormat::decode(reader, v);
			}
			return ret;
		}
		else {
			static_assert(false);
		}
	}

	template<class Writer, class T>
	static constexpr
	auto encode_vector(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		// TODO: Do not cast to size_t, use native type
		auto ret = encode_primitive(writer, static_cast<size_t>(obj.size()));
		if constexpr (c_result<EncodeResult<Writer>>) {
			if (!ret)
				return ret;
			for (const auto& v : obj) {
				auto res = SbsDataFormat::encode(writer, v);
				if (res ) {
					*ret += *res;
				}
				else {
					ret = std::move(res);
					break;
				}
			}
		}
		else if constexpr (std::is_same_v<EncodeResult<Writer>, size_t>) {
			for (const auto& v : obj) {
				ret += SbsDataFormat::encode(writer, v);
			}
		}
		else {
			static_assert(false);
		}
		return ret;
	}

	template<class Reader, class T>
	static constexpr
	auto decode_vector(Reader& reader, T& obj) -> DecodeResult<Reader> {
		size_t size_value;
		auto ret = decode_primitive(reader, size_value);

		if constexpr (c_result<DecodeResult<Reader>>) {
			if (!ret)
				return ret;
			obj.reserve(static_cast<typename T::size_type>(size_value));
			for (auto i = 0zu; i < size_value; ++i) {
				auto& v = obj.emplace_back();
				auto res = SbsDataFormat::decode(reader, v);
				if (!res)
					return res;
				*ret += *res;
			}
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
			obj.reserve(static_cast<typename T::size_type>(size_value));
			for (auto i = 0zu; i < size_value; ++i) {
				auto& v = obj.emplace_back();
				ret += SbsDataFormat::decode(reader, v);
			}
		}
		else {
			static_assert(false);
		}
		return ret;
	}

	template<class Writer, class T>
	static constexpr
	auto encode_set(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		auto ret = encode_primitive(writer, static_cast<size_t>(obj.size()));
		if constexpr (c_result<EncodeResult<Writer>>) {
			if (!ret)
				return ret;
			for (const auto& v : obj) {
				auto res = SbsDataFormat::encode(writer, v);
				if (res ) {
					*ret += *res;
				}
				else {
					ret = std::move(res);
					break;
				}
			}
		}
		else if constexpr (std::is_same_v<EncodeResult<Writer>, size_t>) {
			for (const auto& v : obj) {
				ret += SbsDataFormat::encode(writer, v);
			}
		}
		else {
			static_assert(false);
		}
		return ret;
	}

	template<class Reader, class T>
	static constexpr
	auto decode_set(Reader& reader, T& obj) -> DecodeResult<Reader> {
		size_t size_value;
		auto ret = decode_primitive(reader, size_value);

		if constexpr (c_result<DecodeResult<Reader>>) {
			if (!ret)
				return ret;
			for (auto i = 0zu; i < size_value; ++i) {
				auto v = typename T::key_type{};
				auto res = SbsDataFormat::decode(reader, v);
				if (!res)
					return res;
				obj.insert(std::move(v));
				*ret += *res;
			}
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
			for (auto i = 0zu; i < size_value; ++i) {
				auto v = typename T::key_type{};
				obj.insert(std::move(v));
				ret += SbsDataFormat::decode(reader, v);
			}
		}
		else {
			static_assert(false);
		}
		return ret;
	}

	template<class Writer, class T>
	static constexpr
	auto encode_variant(Writer& writer, const T& obj) -> EncodeResult<Writer> {
		// TODO: Don't encode index if there is only one possible alternative
		using Index = VariantIndexType<T>;
		auto index_value = obj.valueless_by_exception()
			? npos_for<Index>
			: static_cast<Index>(obj.index());
		auto ret = encode_primitive(writer, index_value);

		if constexpr (c_result<EncodeResult<Writer>>) {
			if (!ret)
				return ret;
			if (index_value != npos_for<Index>) {
				std::visit([&](const auto& var) {
					auto res = SbsDataFormat::encode(writer, var);
					if (res)
						*ret += *ret;
					else
						ret = std::move(res);
				}, obj);
			}
		}
		else if constexpr (std::is_same_v<EncodeResult<Writer>, size_t>) {
			if (index_value != npos_for<Index>) {
				std::visit([&](const auto& var) {
					ret += SbsDataFormat::encode(writer, var);
				}, obj);
			}
		}
		else {
			static_assert(false);
		}
		return ret;
	}

	template<class Reader, class T>
	static constexpr
	auto decode_variant(Reader& reader, T& obj) -> DecodeResult<Reader> {
		using Index = VariantIndexType<T>;
		Index index_value;
		auto ret = decode_primitive(reader, index_value);
		if constexpr (c_result<DecodeResult<Reader>>) {
			 if (!ret)
				return ret;
		}

		if (index_value == npos_for<Index>) {
			// There is no direct API to put variant into the valueless state. The workaround
			// is to throw an exception when variant attempts to access some dummy object
			// during emplacement
			if (!obj.valueless_by_exception()) {
				set_variant_valueless(obj);
			}
			return ret;
		}

		if constexpr (c_result<DecodeResult<Reader>>) {
			unroll<mp_size<T>>([&]<size_t I> {
				if (index_value == I) {
					auto& value = obj.index() == I ? get<I>(obj) : obj.template emplace<I>();
					auto res = SbsDataFormat::decode(reader, value);
					if (res)
						*ret += *res;
					else
						ret = std::move(res);
					return false;
				}
				return true;
			});
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
			unroll<mp_size<T>>([&]<size_t I> {
				if (index_value == I) {
					auto& value = obj.index() == I ? get<I>(obj) : obj.template emplace<I>();
					ret += SbsDataFormat::decode(reader, value);
					return false;
				}
				return true;
			});
		}
		else {
			static_assert(false);
		}
		return ret;
	}
};

template<c_byte_writer Writer>
class SbsDataFormat::EncodingArchive {
public:
	static constexpr auto is_encoding = true;
	static constexpr auto is_decoding = false;

	explicit FR_FORCE_INLINE constexpr
	EncodingArchive(Writer& writer) noexcept: _writer(std::addressof(writer)) { }

	template<c_serializable... Args>
	requires (sizeof...(Args) > 0zu)
	FR_FORCE_INLINE constexpr
	auto operator()(const Args&... args) -> EncodeResult<Writer> {
		if constexpr (c_result<EncodeResult<Writer>>) {
			auto ret = EncodeResult<Writer>{0zu};
			(... && [&](const auto& arg) {
				auto res = SbsDataFormat::encode(*_writer, arg);
				if (res) {
					*ret += *res;
					return true;
				}
				else {
					ret = std::move(res);
					return false;
				}
			}(args));
			return ret;
		}
		else if constexpr (std::is_same_v<EncodeResult<Writer>, size_t>) {
			auto sum = 0zu;
			(..., (sum += SbsDataFormat::encode(*_writer, args)));
			return sum;
		}
		else {
			static_assert(false);
		}
	}

private:
	Writer* _writer;
};

template<c_byte_reader Reader>
class SbsDataFormat::DecodingArchive {
public:
	static constexpr auto is_encoding = false;
	static constexpr auto is_decoding = true;

	explicit FR_FORCE_INLINE constexpr
	DecodingArchive(Reader& reader) noexcept: _reader(std::addressof(reader)) { }

	template<class... Args>
	FR_FORCE_INLINE constexpr
	auto operator()(Args&&... args) -> DecodeResult<Reader> {
		if constexpr (c_result<DecodeResult<Reader>>) {
			auto ret = DecodeResult<Reader>{0zu};
			(... && [&](auto& arg) {
				auto res = SbsDataFormat::decode(*_reader, arg);
				if (res) {
					*ret += *res;
					return true;
				}
				else {
					ret = std::move(res);
					return false;
				}
			}(args));
			return ret;
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader>, size_t>) {
			auto sum = 0zu;
			(..., (sum += SbsDataFormat::decode(*_reader, args)));
			return sum;
		}
		else {
			static_assert(false);
		}
		(..., SbsDataFormat::decode(*_reader, args));
	}

private:
	Reader* _reader;
};

} // namespace fr
#endif // include guard
