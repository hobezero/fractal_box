#ifndef FRACTAL_BOX_CORE_PREPROCESSOR_HPP
#define FRACTAL_BOX_CORE_PREPROCESSOR_HPP

// String utilities
// ----------------

#define FR_DETAIL_STRINGIFY(x) #x
#define FR_TO_STRING(x) FR_DETAIL_STRINGIFY(x)

#define FR_DETAIL_CONCAT_IMPL(a, b) a##b
#define FR_CONCAT(a, b) FR_DETAIL_CONCAT_IMPL(a, b)

#define FR_ARR_STRLEN(x) (sizeof(x)/sizeof((x)[0]))
#define FR_LITERAL_STRLEN(x) FR_ARR_STRLEN(FR_TO_STRING(x))

// Name generation
// ---------------

#ifdef __COUNTER__
#	define FR_UNIQUE_NAME(base) FR_CONCAT(base, __COUNTER__)
#else
#	define FR_UNIQUE_NAME(base) FR_CONCAT(base, __LINE__)
#endif

// Conditional expansion
// ---------------------

#define FR_DETAIL_CONDITIONAL_0(true_expr, false_expr) false_expr
#define FR_DETAIL_CONDITIONAL_1(true_expr, false_expr) true_expr

/// @note cond_bit must be either 0 or 1
#define FR_CONDITIONAL(cond_bit, true_expr, false_expr) \
	FR_CONCAT(FR_DETAIL_CONDITIONAL_, cond_bit(true_expr, false_expr))

// Utilities for variadic arguments
// --------------------------------

#define FR_VA_ZERO(...) 0

#define FR_DETAIL_COUNT_VA( \
	a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, \
	a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, \
	a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, \
	a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, \
	a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, \
	a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, \
	a60, a61, a62, a63, \
	count, ... \
) count

/// @brief Expand to the non-zero number of variadic macro arguments passed to it
/// @note The list of arguments must not be empty
#define FR_VA_SIZE_NON_EMPTY(...) \
	FR_DETAIL_COUNT_VA( \
		__VA_ARGS__, \
		64, 63, 62, 61, \
		60, 59, 58, 57, 56, 55, 54, 53, 52, 51, \
		50, 49, 48, 47, 46, 45, 44, 43, 42, 41, \
		40, 39, 38, 37, 36, 35, 34, 33, 32, 31, \
		30, 29, 28, 27, 26, 25, 24, 23, 22, 21, \
		20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
		10,  9,  8,  7,  6,  5,  4,  3,  2,  1, \
	)

// TODO: Investigate if inidirection is needed
#define FR_DETAIL_VA_HEAD_IMPL(a0, ...) a0

/// @brief Expand to the first argument
#define FR_VA_HEAD(a0, ...) FR_DETAIL_VA_HEAD_IMPL(a0, __VA_ARGS__)

// TODO: Investigate if inidirection is needed
#define FR_DETAIL_EMPTY_VA_INDICATOR_IMPL(...) __VA_OPT__(0,) 1
#define FR_DETAIL_EMPTY_VA_INDICATOR(...) FR_DETAIL_EMPTY_VA_INDICATOR_IMPL(__VA_ARGS__)

/// @brief Expand to 1 if the given list of variadic macro arguments is empty, 0 otherwise
#define FR_IS_VA_EMPTY(...) \
	FR_VA_HEAD(FR_DETAIL_EMPTY_VA_INDICATOR(__VA_ARGS__),)

/// @brief Expand to the number of variadic macro arguments passed to it. Supports zero arguments
#define FR_VA_SIZE(...) \
	FR_CONDITIONAL( \
		FR_IS_VA_EMPTY(__VA_ARGS__), \
		FR_VA_ZERO, \
		FR_VA_SIZE_NON_EMPTY \
	)(__VA_ARGS__)

/// @brief Select one of many "overloaded" macros depending on the number of arguments
/// @example Usage:
///   ```cpp
///   #define FOO_1() /* ... */
///   #define FOO_2(a) /* ... */
///   #define FOO(...) FR_OVERLOAD_MACRO(FOO_, __VA_ARGS__)(__VA_ARGS__)
///   ```
/// @see BOOST_PP_OVERLOAD in Boost.Preprocessor
#define FR_OVERLOAD_MACRO(macro, ...) \
	FR_CONCAT(macro, FR_VA_SIZE(__VA_ARGS__))

// Explicitly silence some compiler warnings
// -----------------------------------------

#define FR_DETAIL_UNUSED_1(a0) \
	static_cast<void>(a0)
#define FR_DETAIL_UNUSED_2(a0, a1) \
	static_cast<void>(a0); static_cast<void>(a1)
#define FR_DETAIL_UNUSED_3(a0, a1, a2) \
	static_cast<void>(a0); static_cast<void>(a1); static_cast<void>(a2)
#define FR_DETAIL_UNUSED_4(a0, a1, a2, a3) \
	static_cast<void>(a0); static_cast<void>(a1); static_cast<void>(a2); static_cast<void>(a3)
#define FR_DETAIL_UNUSED_5(a0, a1, a2, a3, a4) \
	static_cast<void>(a0); static_cast<void>(a1); static_cast<void>(a2); static_cast<void>(a3); \
	static_cast<void>(a4)
#define FR_DETAIL_UNUSED_6(a0, a1, a2, a3, a4, a5) \
	static_cast<void>(a0); static_cast<void>(a1); static_cast<void>(a2); static_cast<void>(a3); \
	static_cast<void>(a4); static_cast<void>(a5)
#define FR_DETAIL_UNUSED_7(a0, a1, a2, a3, a4, a5, a6) \
	static_cast<void>(a0); static_cast<void>(a1); static_cast<void>(a2); static_cast<void>(a3); \
	static_cast<void>(a4); static_cast<void>(a5); static_cast<void>(a6)
#define FR_DETAIL_UNUSED_8(a0, a1, a2, a3, a4, a5, a6, a7) \
	static_cast<void>(a0); static_cast<void>(a1); static_cast<void>(a2); static_cast<void>(a3); \
	static_cast<void>(a4); static_cast<void>(a5); static_cast<void>(a6); static_cast<void>(a7)
#define FR_DETAIL_UNUSED_9(a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11) \
	static_cast<void>(a00); static_cast<void>(a01); static_cast<void>(a02); static_cast<void>(a03);\
	static_cast<void>(a04); static_cast<void>(a05); static_cast<void>(a06); static_cast<void>(a07);\
	static_cast<void>(a08);
#define FR_DETAIL_UNUSED_10(a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11) \
	static_cast<void>(a00); static_cast<void>(a01); static_cast<void>(a02); static_cast<void>(a03);\
	static_cast<void>(a04); static_cast<void>(a05); static_cast<void>(a06); static_cast<void>(a07);\
	static_cast<void>(a08); static_cast<void>(a09);
#define FR_DETAIL_UNUSED_11(a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11) \
	static_cast<void>(a00); static_cast<void>(a01); static_cast<void>(a02); static_cast<void>(a03);\
	static_cast<void>(a04); static_cast<void>(a05); static_cast<void>(a06); static_cast<void>(a07);\
	static_cast<void>(a08); static_cast<void>(a09); static_cast<void>(a10);
#define FR_DETAIL_UNUSED_12(a00, a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11) \
	static_cast<void>(a00); static_cast<void>(a01); static_cast<void>(a02); static_cast<void>(a03);\
	static_cast<void>(a04); static_cast<void>(a05); static_cast<void>(a06); static_cast<void>(a07);\
	static_cast<void>(a08); static_cast<void>(a09); static_cast<void>(a10); static_cast<void>(a11)
// Nobody needs more parameters, right?

/// @brief Silence "unused" warnings similiar to `[[maybe_unused]]`, but works on expressions
/// rather than declarations. Supports up to 12 arguments
/// @note Arguments are always evaluated
#define FR_UNUSED(...) FR_OVERLOAD_MACRO(FR_DETAIL_UNUSED_, __VA_ARGS__)(__VA_ARGS__)

/// @brief Silence warnings when ignoring a return value from a `[[nodiscard]]` function
#define FR_IGNORE(result) static_cast<void>(result)

// TODO: Support #pragma(push_macro)

#endif // include guard
