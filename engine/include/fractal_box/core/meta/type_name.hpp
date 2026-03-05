#ifndef FRACTAL_BOX_CORE_META_TYPE_NAME_HPP
#define FRACTAL_BOX_CORE_META_TYPE_NAME_HPP

#include <string_view>

#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/hashing/fnv.hpp"
#include "fractal_box/core/string_literal.hpp"

/// @brief DO NOT USE downstream, this is an implementation detail
/// @note Has to be in the global namespace
struct fr_detail_ReflStruct {
	enum class ReflEnum {
		ReflValue
	};

public:
	void* _refl_member;
};

namespace fr {

namespace detail {

template<class T>
struct FakeWrapper {
	const T value;
};

FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_UNDEFINED_VAR_TEMPLATE

template<class T>
extern const FakeWrapper<T> fake_object_assert;

template<class T>
inline constexpr
auto fake_object() noexcept -> const T& {
	return fake_object_assert<T>.value;
}

FR_DIAGNOSTIC_POP

template<class... Ts>
inline consteval
auto pretty_func_name() noexcept -> std::string_view {
#if FR_COMP_CLANG || FR_COMP_GCC_EMULATED
	return __PRETTY_FUNCTION__;
#elif FR_COMP_MSVC_EMULATED
	return __FUNCSIG__;
#else
	return std::source_location::current().function_name();
#endif
}

template<auto... Vs>
inline consteval
auto pretty_func_name() noexcept -> std::string_view {
#if FR_COMP_CLANG || FR_COMP_GCC_EMULATED
	return __PRETTY_FUNCTION__;
#elif FR_COMP_MSVC_EMULATED
	return __FUNCSIG__;
#else
	return std::source_location::current().function_name();
#endif
}

struct TypeNameHelper {
	static constexpr auto test_name = pretty_func_name<double>();
	static constexpr char test_str[] = "double";
	static constexpr size_t start_pos = test_name.find(test_str);
	static_assert(start_pos != std::string_view::npos);
	static constexpr size_t suffix_len = test_name.size() - start_pos - (std::size(test_str) - 1zu);
};

struct ClassNameHelper {
	static constexpr auto test_name = pretty_func_name<::fr_detail_ReflStruct>();
	static constexpr char test_str[] = "fr_detail_ReflStruct";
	static constexpr size_t start_pos = test_name.find(test_str);
	static_assert(start_pos != std::string_view::npos);
	static constexpr size_t suffix_len = test_name.size() - start_pos - (std::size(test_str) - 1zu);
};

struct EnumNameHelper {
	static constexpr auto test_name = pretty_func_name<::fr_detail_ReflStruct::ReflEnum>();
	static constexpr char test_str[] = "fr_detail_ReflStruct::ReflEnum";
	static constexpr size_t start_pos = test_name.find(test_str);
	static_assert(start_pos != std::string_view::npos);
	static constexpr size_t suffix_len = test_name.size() - start_pos - (std::size(test_str) - 1zu);
};

struct MemberNameHelper {
	static constexpr auto test_name = pretty_func_name<
		&fr_detail_ReflStruct::_refl_member>();
	static constexpr char test_str[] = "&fr_detail_ReflStruct::_refl_member";
	static constexpr size_t start_pos = test_name.find(test_str);
	static_assert(start_pos != std::string_view::npos);
	static constexpr size_t suffix_len = test_name.size() - start_pos - (std::size(test_str) - 1zu);
};

struct MemberNameHelper2 {
	static constexpr auto test_name = pretty_func_name<
		&fake_object<fr_detail_ReflStruct>()._refl_member>();
	static constexpr char test_str[] = "_refl_member";
	static constexpr size_t start_pos = test_name.find(test_str);
	static_assert(start_pos != std::string_view::npos);
	static constexpr size_t suffix_len = test_name.size() - start_pos - (std::size(test_str) - 1zu);
	static_assert(suffix_len > 0zu);
};

template<class T, class Helper>
inline consteval
auto get_type_name() noexcept {
	static constexpr auto func_name = pretty_func_name<T>();
	static constexpr auto slice = func_name.substr(
		Helper::start_pos,
		func_name.size() - Helper::start_pos - Helper::suffix_len
	);

	static_assert(slice.size() > 0zu);
	return StringLiteral<char, slice.size()>{slice};
}

template<class T, class Helper>
inline consteval
auto get_unqualified_type_name() noexcept {
	using PT = std::remove_pointer_t<std::remove_cvref_t<T>>;
	static constexpr auto func_name = pretty_func_name<PT>();

	static constexpr auto quali_name = func_name.substr(
		Helper::start_pos,
		func_name.size() - Helper::start_pos - Helper::suffix_len
	);
	static constexpr auto tmpl_name = quali_name.substr(0zu, quali_name.find_first_of("<", 1zu));
	static constexpr auto unquali_name = quali_name.substr(tmpl_name.find_last_of("::") + 1zu);

	static_assert(unquali_name.size() > 0zu);
	return StringLiteral<char, unquali_name.size()>{unquali_name};
}

template<class T, class Helper>
inline consteval
auto get_clean_type_name() noexcept {
	using PT = std::remove_pointer_t<std::remove_cvref_t<T>>;
	static constexpr auto func_name = pretty_func_name<PT>();

	static constexpr auto quali_name = func_name.substr(
		Helper::start_pos,
		func_name.size() - Helper::start_pos - Helper::suffix_len
	);
	static constexpr auto tmpl_name = quali_name.substr(0zu, quali_name.find_first_of("<", 1zu));
	static constexpr auto clean_name = tmpl_name.substr(tmpl_name.find_last_of("::") + 1zu);

	static_assert(clean_name.size() > 0zu);
	return StringLiteral<char, clean_name.size()>{clean_name};
}

template<auto Member>
inline consteval
auto get_member_name() noexcept {
	// NOTE: qlibs/reflect does something much more complicated for no apparent reason
	static constexpr auto func_name = pretty_func_name<Member>();
	static constexpr auto slice = func_name.substr(
		MemberNameHelper::start_pos,
		func_name.size() - MemberNameHelper::start_pos - MemberNameHelper::suffix_len
	);
	static constexpr auto clean_name = slice.substr(slice.find_last_of("::") + 1zu);

	static_assert(clean_name.size() > 0zu);
	return StringLiteral<char, clean_name.size()>{clean_name};
}

} // namespace detail

template<class T>
inline constexpr auto type_name_lit = detail::get_type_name<T, detail::TypeNameHelper>();

template<c_class T>
inline constexpr auto type_name_lit<T> = detail::get_type_name<T, detail::ClassNameHelper>();

template<c_enum T>
inline constexpr auto type_name_lit<T> = detail::get_type_name<T, detail::EnumNameHelper>();

template<class T>
inline constexpr auto unqualified_type_name_lit
	= detail::get_unqualified_type_name<T, detail::TypeNameHelper>();

template<c_class T>
inline constexpr auto unqualified_type_name_lit<T>
	= detail::get_unqualified_type_name<T, detail::ClassNameHelper>();

template<c_enum T>
inline constexpr auto unqualified_type_name_lit<T>
	= detail::get_unqualified_type_name<T, detail::EnumNameHelper>();

template<class T>
inline constexpr auto clean_type_name_lit
	= detail::get_clean_type_name<T, detail::TypeNameHelper>();

template<c_class T>
inline constexpr auto clean_type_name_lit<T>
	= detail::get_clean_type_name<T, detail::ClassNameHelper>();

template<c_enum T>
inline constexpr auto clean_type_name_lit<T>
	= detail::get_clean_type_name<T, detail::EnumNameHelper>();

template<auto Member>
inline constexpr auto member_name_lit = detail::get_member_name<Member>();

/// @brief An implementation-defined string representation of a type name
/// @note Global variable that actually stores type name characters at runtime
/// @note Defined this way to reduce the size of the string data, as well as the size of the mangled
/// symbol
template<class T>
inline constexpr auto type_name = std::string_view{type_name_lit<T>};

template<class T>
inline constexpr auto unqualified_type_name = std::string_view{unqualified_type_name_lit<T>};

template<class T>
inline constexpr auto clean_type_name = std::string_view{clean_type_name_lit<T>};

template<auto Member>
inline constexpr auto member_name = std::string_view{member_name_lit<Member>};

template<c_hash_digest Digest, class T>
inline constexpr auto type_hash = fnv1a_hash_string<Digest>(type_name_lit<T>.data(),
	type_name_lit<T>.size());

template<class T>
inline constexpr auto type_hash32 = type_hash<HashDigest32, T>;

template<class T>
inline constexpr auto type_hash64 = type_hash<HashDigest64, T>;

} // namespace fr
#endif // include guard
