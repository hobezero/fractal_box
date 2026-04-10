#ifndef FRACTAL_BOX_CORE_SERIALIZATION_SBS_DATA_FORMAT_HPP
#define FRACTAL_BOX_CORE_SERIALIZATION_SBS_DATA_FORMAT_HPP

#include <cstring>

#include <utility>

#include "fractal_box/core/byte_utils.hpp"
#include "fractal_box/core/containers/simple_array.hpp"
#include "fractal_box/core/io/io_concepts.hpp"
#include "fractal_box/core/serialization/serialization_concepts.hpp"

namespace fr {

/// @brief Simple binary serialization format
struct SbsDataFormat {
private:
	static_assert(std::endian::native == std::endian::little);

	template<class Writer, class T>
	static consteval
	auto calc_encode_result_type() noexcept {
		using Char = typename Writer::CharType;
		using WriteResult = decltype(std::declval<Writer&>().write(
			std::declval<std::span<const Char>>()));
		return mp_type<WriteResult>;
	}

	template<class Reader, class T>
	static consteval
	auto calc_decode_result_type() noexcept {
		using Char = typename Reader::CharType;
		using ReadResult = decltype(std::declval<Reader&>().read(std::declval<std::span<Char>>()));
		return mp_type<ReadResult>;
	}

public:
	template<c_byte_writer Writer, c_serializable T>
	using EncodeResult = typename decltype(calc_encode_result_type<Writer, T>())::Type;

	template<c_byte_reader Reader, c_serializable T>
	using DecodeResult = typename decltype(calc_decode_result_type<Reader, T>())::Type;

	template<c_byte_writer Writer, c_serializable T>
	static FR_FORCE_INLINE constexpr
	auto encode(Writer& writer, const T& obj) -> EncodeResult<Writer, T> {
		using enum SerializableCategory;
		static constexpr auto serializability = get_serializability<T>();
		if constexpr (serializability) {
			if constexpr (serializability.category() == Primitive) {
				return encode_primitive(writer, obj);
			}
			else if constexpr (serializability.category() == Custom) {
			}
			else if constexpr (serializability.category() == Described) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Enum) {
				return encode_enum(writer, obj);
			}
			else if constexpr (serializability.category() == Optional) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == String) {
				return encode_string(writer, obj);
			}
			else if constexpr (serializability.category() == Array) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Vector) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Map) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Set) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Variant) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Record) {
				static_assert(false);
			}
			else {
				static_assert(false);
			}
		}
		else {
			static_assert(false);
		}
	}

	template<c_byte_reader Reader, c_serializable T>
	static FR_FORCE_INLINE constexpr
	auto decode(Reader& reader, T& obj) -> DecodeResult<Reader, T> {
		using enum SerializableCategory;
		static constexpr auto serializability = get_serializability<T>();
		if constexpr (serializability) {
			if constexpr (serializability.category() == Primitive) {
				return decode_primitive(reader, obj);
			}
			else if constexpr (serializability.category() == Custom) {
			}
			else if constexpr (serializability.category() == Described) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Enum) {
				return decode_enum(reader, obj);
			}
			else if constexpr (serializability.category() == Optional) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == String) {
				return decode_string(reader, obj);
			}
			else if constexpr (serializability.category() == Array) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Vector) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Map) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Set) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Variant) {
				static_assert(false);
			}
			else if constexpr (serializability.category() == Record) {
				static_assert(false);
			}
			else {
				static_assert(false);
			}
		}
	}

private:
	template<class T>
	static FR_FORCE_INLINE constexpr
	auto result_size(const T& result) noexcept -> size_t {
		if constexpr (std::is_same_v<T, size_t>) {
			return result;
		}
		else if constexpr (is_result_of<T, size_t>) {
			FR_ASSERT_AUDIT(result);
			return result.value();
		}
		else {
			static_assert(false);
		}
	}

	// Primitives
	// ^^^^^^^^^^

	template<class Writer, class T>
	static constexpr
	auto encode_primitive(Writer& writer, const T& obj) -> EncodeResult<Writer, T> {
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
	auto decode_primitive(Reader& reader, T& obj) -> DecodeResult<Reader, T> {
		using Char = typename Reader::CharType;
		if constexpr (std::is_arithmetic_v<T>) {
			if consteval {
				auto obj_bytes = SimpleArray<Char, sizeof(T)>{};
				if constexpr (c_result<DecodeResult<Reader, T>>) {
					auto res = reader.read_exact(std::span<Char>(obj_bytes.data(), sizeof(T)));
					if (!res)
						return {from_error, std::move(res)};
					obj = std::bit_cast<T>(obj_bytes);
					return {in_place, sizeof(T)};
				}
				else if constexpr (std::is_same_v<DecodeResult<Reader, T>, size_t>) {
					reader.read_exact(std::span<Char>(obj_bytes.data(), sizeof(T)));
					obj = std::bit_cast<T>(obj_bytes);
					return sizeof(T);
				}
				else {
					static_assert(false);
				}
			}
			else {
				if constexpr (c_result<DecodeResult<Reader, T>>) {
					auto res = reader.read_exact(std::span<Char>(reinterpret_cast<Char*>(&obj),
						sizeof(T)));
					if (!res)
						return {from_error, std::move(res)};
					return {in_place, sizeof(T)};
				}
				else if constexpr (std::is_same_v<DecodeResult<Reader, T>, size_t>) {
					reader.read_exact(std::span<Char>(reinterpret_cast<Char*>(&obj), sizeof(T)));
					return sizeof(T);
				}
				else {
					return sizeof(T);
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
	auto encode_enum(Writer& writer, E obj) -> EncodeResult<Writer, E> {
		return encode_primitive(writer, std::to_underlying(obj));
	}

	template<class Reader, class E>
	static FR_FORCE_INLINE constexpr
	auto decode_enum(Reader& reader, E& obj) -> DecodeResult<Reader, E> {
		std::underlying_type_t<E> value;
		auto result = decode_primitive(reader, value);
		obj = static_cast<E>(value);
		return result;
	}

	// Strings
	// ^^^^^^^

	template<class Writer, class T>
	static constexpr
	auto encode_string(Writer& writer, const T& obj) -> EncodeResult<Writer, T> {
		auto size_res = encode_primitive(writer, static_cast<size_t>(obj.size()));
		if (!size_res)
			return size_res;

		using WriterChar = typename Writer::CharType;
		using StringChar = typename T::value_type;
		if constexpr (std::is_same_v<WriterChar, StringChar>) {
			auto data_res = writer.write(std::span<const WriterChar>(obj.data(), obj.size()));
			if (!data_res)
				return data_res;
			return EncodeResult<Writer, T>{result_size(size_res) + result_size(data_res)};
		}
		else {
			const auto bytes_size = sizeof(StringChar) * obj.size();
			if consteval {
				auto* bytes = new WriterChar[bytes_size];
				write_str_as_bytes(bytes, obj.data(), obj.size());
				auto data_res = writer.write(std::span<const WriterChar>(bytes, bytes_size));
				delete[] bytes;
				if (!data_res)
					return data_res;
				return EncodeResult<Writer, T>{result_size(size_res) + result_size(data_res)};
			}
			else {
				auto data_res = writer.write(std::span<const WriterChar>(
					reinterpret_cast<const WriterChar*>(obj.data()),
					bytes_size
				));
				if (!data_res)
					return data_res;
				return EncodeResult<Writer, T>{result_size(size_res) + result_size(data_res)};
			}
		}
	}

	template<class Reader, class T>
	static constexpr
	auto decode_string(Reader& reader, T& obj) -> DecodeResult<Reader, T> {
		size_t size_value;
		auto size_res = decode_primitive(reader, size_value);
		if (!size_res)
			return size_res;

		using ReaderChar = typename Reader::CharType;
		using StringChar = typename T::value_type;

		const auto old_size = obj.size();
		const auto bytes_size = sizeof(StringChar) * size_value;
		if constexpr (c_result<DecodeResult<Reader, T>>) {
			using ReadExactResult = typename DecodeResult<Reader, T>::template Rebind<void>;
			union U {
				constexpr
				U() noexcept { }

			public:
				ReadExactResult v;
			} data_res;

			if constexpr (std::is_same_v<ReaderChar, StringChar>) {
				obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
					std::construct_at(std::addressof(data_res.v),
						reader.read_exact(std::span<ReaderChar>(data, n)));
					return n;
				});
			}
			else {
				if consteval {
					obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
						auto* bytes = new ReaderChar[bytes_size];
						std::construct_at(std::addressof(data_res.v),
							reader.read_exact(std::span<ReaderChar>(bytes, bytes_size)));
						if (data_res.v)
							read_str_from_bytes(data, n, bytes);
						delete[] bytes;
						return n;
					});
				}
				else {
					obj.resize_and_overwrite(size_value, [&](StringChar* data, size_t n) {
						std::construct_at(std::addressof(data_res.v),
							reader.read_exact(std::span<ReaderChar>(
								reinterpret_cast<ReaderChar*>(data), bytes_size)));
						return n;
					});
				}
			}

			if (!data_res.v) {
				obj.resize(old_size);
				return {from_error, std::move(data_res).v};
			}
			return {in_place, result_size(size_res) + bytes_size};
		}
		else if constexpr (std::is_same_v<DecodeResult<Reader, T>, size_t>) {
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
};

} // namespace fr
#endif // include guard
