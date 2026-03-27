#ifndef FRACTAL_BOX_CORE_IO_IO_CONCEPTS_HPP
#define FRACTAL_BOX_CORE_IO_IO_CONCEPTS_HPP

#include <span>

#include "fractal_box/core/byte_utils.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/error_handling/result.hpp"

namespace fr {

template<class T>
concept c_io_character = c_byte_like<T> || c_character<T>;

template<class T>
concept c_writer
	= c_user_object<T>
	&& requires(T& obj, size_t size, std::span<const typename T::CharType> const_span) {
		requires c_io_character<typename T::CharType>;

		{ obj.write(const_span) } -> c_size_or_result;
		{ obj.flush() } -> c_void_or_result;

		{ T::is_buffered } -> c_similar_to<bool>;
		{ T::is_buffer_resizable } -> c_similar_to<bool>;
		requires !T::is_buffered || requires {
			{ obj.buffer() } -> std::same_as<std::span<typename T::CharType>>;
			{ obj.commit_buffer(size) } -> c_void_or_result;
		};
		requires !T::is_buffer_resizable || requires {
			// Returns new buffer size
			{ obj.resize_buffer(size) } -> c_size_or_result;
		};
	};

template<class T>
concept c_reader
	= c_user_object<T>
	&& requires(T& obj, size_t size, std::span<typename T::CharType> mut_span) {
		requires c_io_character<typename T::CharType>;

		{ obj.read(mut_span) } -> c_size_or_result;

		{ T::is_buffered } -> c_similar_to<bool>;
		requires !T::is_buffered || requires {
			{ obj.buffer() } -> std::same_as<std::span<const typename T::CharType>>;
			{ obj.commit_buffer(size) } -> c_void_or_result;
		};
	};

struct Eof { };

} // namespace fr
#endif
