#ifndef FRACTAL_BOX_CORE_PLATFORM_HPP
#define FRACTAL_BOX_CORE_PLATFORM_HPP

/// @file
/// @brief Macros for platform-specific, compiler-specific features and feature detection
///
/// Inspired by Boost.Predef but based on Hedley library
/// Usage:
///   ```cpp
///   #if FR_COMP_CLANG
///   // clang detected
///   #endif
///   ```
/// @todo
///   TODO: Convert to C instead of C++ to make this header usable from C sources.
///         Don't forget to use .h suffix!
///   TODO: Custom version macro? Hedley and Boost.Predef use different version encoding schemes
///   TODO: Remove the dependency on Hedley. It's a dead project anyway

#include <hedley.h>

// Compiler detection
// ------------------

#define FR_VERSION_ENCODE(major, minor, patch) HEDLEY_VERSION_ENCODE(major, minor, patch)

// Clang
#ifdef __clang__
#	define FR_COMP_CLANG \
		FR_VERSION_ENCODE(__clang_major__, __clang_minor__, __clang_patchlevel__)

#	ifdef __llvm__ // Detect Clang/LLVM
#		define FR_COMP_CLANG_LLVM FR_COMP_CLANG
#	else
#		define FR_COMP_CLANG_LLVM 0
#	endif

#	ifdef _MSC_VER // Detect Clang-cl
#		define FR_COMP_CLANG_CL FR_COMP_CLANG
#	else
#		define FR_COMP_CLANG_CL 0
#	endif

#	ifdef __apple_build_version__ // Detect AppleClang
#		define FR_COMP_CLANG_APPLE FR_COMP_CLANG
#	else
#		define FR_COMP_CLANG_APPLE 0
#	endif
#else
#	define FR_COMP_CLANG 0
#	define FR_COMP_CLANG_LLVM 0
#	define FR_COMP_CLANG_CL 0
#	define FR_COMP_CLANG_APPLE 0
#endif

// GCC emulated (e.g., Clang, GCC, ...)
#ifdef HEDLEY_GNUC_VERSION
#	define FR_COMP_GCC_EMULATED HEDLEY_GNUC_VERSION
#else
#	define FR_COMP_GCC_EMULATED 0
#endif

// GCC proper
#ifdef HEDLEY_GCC_VERSION
#	define FR_COMP_GCC HEDLEY_GCC_VERSION
#else
#	define FR_COMP_GCC 0
#endif

// MSVC emulated (e.g. Miscrosoft compiler or clang-cl)
#ifdef HEDLEY_MSVC_VERSION
#	define FR_COMP_MSVC_EMULATED HEDLEY_MSVC_VERSION
#else
#	define FR_COMP_MSVC_EMULATED 0
#endif

// MSVC proper
#if FR_COMP_MSVC_EMULATED && !FR_COMP_CLANG
#	define FR_COMP_MSVC HEDLEY_MSVC_VERSION
#else
#	define FR_COMP_MSVC 0
#endif

// Emscripten
#ifdef HEDLEY_EMSCRIPTEN_VERSION
#	define FR_COMP_EMSCRIPTEN HEDLEY_EMSCRIPTEN_VERSION
#else
#	define FR_COMP_EMSCRIPTEN 0
#endif

// Operating system detection
// --------------------------

// Windows
#if (defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) \
	|| defined(__TOS_WIN__) || defined(WINDOWS) \
)
#	define FR_OS_WINDOWS 1
#else
#	define FR_OS_WINDOWS 0
#endif

// Unix
#if (defined(unix) || defined(__unix) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE))
#	define FR_OS_UNIX 1
#else
#	define FR_OS_UNIX 0
#endif

// Linux
#if (defined(linux) || defined(__linux) \
	|| defined(__linux__) || defined(__gnu_linux__) \
)
#	define FR_OS_LINUX 1
#else
#	define FR_OS_LINUX 0
#endif

// iOS
#if (defined(__APPLE__) && defined (__MACH__) \
	&& defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) \
)
	// TODO: Consider changing version number
#	define FR_OS_IOS (__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__*1000)
#else
#	define FR_OS_IOS 0
#endif

// MacOS
#if (defined(macintosh) || defined(Macintosh) \
	|| (defined(__APPLE__) && defined(__MACH__)) \
)
	// TODO: Detect MacOS 11, 12, 13... using `Availability.h`/`AvailabilityMacros.h` headers
	// (https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/cross_development/Using/using.html)
#	if defined(__APPLE__) && defined(__MACH__)
#		define FR_OS_MACOS HEDLEY_VERSION_ENCODE(10,0,0)
#	else
#		define FR_OS_MACOS HEDLEY_VERSION_ENCODE(9,0,0)
#	endif
#else
#	define FR_OS_MACOS 0
#endif

// TODO: Detect Android

// CPU architecture detection
// --------------------------

// i386 (32-bit version of x86 specifically)
#if (defined(i386) || defined(__i386__) || defined(__i386) \
	|| defined(__i486__) || defined(__i586__) || defined(__i686__) \
	|| defined(_M_IX86) || defined(_X86_) \
	|| defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__) \
)
#	define FR_ARCH_x86_32 1
#else
#	define FR_ARCH_x86_32 0
#endif

// x86_64
#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) \
	|| defined(__amd64__) || defined(__amd64) \
)
#	define FR_ARCH_x86_64 1
#else
#	define FR_ARCH_x86_64 0
#endif

// ARM
#if (defined(__ARM_ARCH) || defined(__TARGET_ARCH_ARM) \
	|| defined(__TARGET_ARCH_THUMB) || defined(_M_ARM) \
	|| defined(__arm__) || defined(__arm64) \
	|| defined(__thumb__) \
	|| defined(_M_ARM64) || defined(__aarch64__) || defined(__AARCH64EL__) \
	|| defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) \
	|| defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) \
	|| defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) \
	|| defined(__ARM_ARCH_6KZ__) || defined(__ARM_ARCH_6T2__) \
	|| defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__) \
	|| defined(__ARM_ARCH_4T__) || defined(__ARM_ARCH_4__) \
)
#	define FR_ARCH_ARM 1
#else
#	define FR_ARCH_ARM 0
#endif

// ARM64 (AArch64)
#if defined(__arm64) || defined(_M_ARM64) || defined(__aarch64__) || defined(__AARCH64EL__)
#	define FR_ARCH_ARM64 1
#else
#	define FR_ARCH_ARM64 0
#endif

// PowerPC
#if (defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) \
	|| defined(__POWERPC__) || defined(__ppc__) || defined(__ppc64__) || defined(__ppc) \
	|| defined(__PPC__) || defined(__PPC64__) \
	|| defined(_M_PPC) || defined(_ARCH_PPC) || defined(_ARCH_PPC64) \
	|| defined(__PPCGECKO__) || defined(__PPCBROADWAY__) \
	|| defined(_XENON) \
)
#	define FR_ARCH_POWER_PC 1
#else
#	define FR_ARCH_POWER_PC 0
#endif

// SPARC
#if defined(__sparc__) || defined(__sparc)
#	define FR_ARCH_SPARC 1
#else
#	define FR_ARCH_SPARC 0
#endif

// MIPS
#if defined(__mips__) || defined(__mips) || defined(__MIPS__)
#	define FR_ARCH_MIPS 1
#else
#	define FR_ARCH_MIPS 0
#endif


// Built-in types & properties
// ---------------------------

#ifdef __SIZEOF_INT128__
#	define FR_HAS_INT128 1
#else
#	define FR_HAS_INT128 0
#endif

#if FR_COMP_MSVC || defined(__LITTLE_ENDIAN__) \
		|| (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#	define FR_LITTLE_ENDIAN 1
#	define FR_BIG_ENDIAN 0
#elif  defined(__BIG_ENDIAN__) \
	|| (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#	define FR_LITTLE_ENDIAN 0
#	define FR_BIG_ENDIAN 1
#else
#	define FR_LITTLE_ENDIAN 0
#	define FR_BIG_ENDIAN 0
#endif


// Compiler feature queries
// ------------------------

#define FR_HAS_ATTRIBUTE HEDLEY_HAS_ATTRIBUTE

// Optimization attributes/hints
// -----------------------------

#define FR_FORCE_INLINE HEDLEY_ALWAYS_INLINE

#if FR_COMP_GCC_EMULATED && FR_HAS_ATTRIBUTE(always_inline)
#define FR_HAS_FORCE_INLINE_L 1
#define FR_FORCE_INLINE_L __attribute__((always_inline))
#elif FR_COMP_MSVC
#define FR_HAS_FORCE_INLINE_L 1
#define FR_FORCE_INLINE_L [[msvc::forceinline]]
#endif

#if FR_COMP_CLANG || FR_COMP_GCC
#	define FR_HAS_NOINLINE 1
#	define FR_NOINLINE __attribute__((noinline))
#elif FR_COMP_MSVC
#	define FR_HAS_NOINLINE 1
	// Microsoft documents `noinline` only for member functions, but it seems to work for free
	// functions as well
#	define FR_NOINLINE __declspec(noinline)
#else
#	define FR_HAS_NOINLINE 0
#	define FR_NOINLINE
#endif

#if FR_COMP_CLANG || FR_COMP_GCC
#	define FR_HAS_FLATTEN 1
#	define FR_FLATTEN __attribute__((flatten))
#else
#	define FR_HAS_FLATTEN 0
#	define FR_FLATTEN
#endif

// NOTE: Explicit cast ensures that the resulting expression has type `bool`, preventing
// issues in C++ mode with function overloading, type deduction, template instantiation, etc.
// Hedley, Boost.Assert, etc. fall short in this regard.
// NOTE: We don't use the C++20 attributes `[[likely]]` and `[[unlikely]]` because they are broken
// (https://blog.aaronballman.com/2020/08/dont-use-the-likely-or-unlikely-attributes)

#if FR_COMP_GCC || HEDLEY_HAS_BUILTIN(__builtin_expect)
#	define FR_HAS_LIKELY 1
#	define FR_HAS_UNLIKELY 1

#	define FR_LIKELY(expr) \
		HEDLEY_STATIC_CAST(bool, __builtin_expect(HEDLEY_STATIC_CAST(bool, (expr)), 1))
#	define FR_UNLIKELY(expr) \
		HEDLEY_STATIC_CAST(bool, __builtin_expect(HEDLEY_STATIC_CAST(bool, (expr)), 0))
#else
#	define FR_HAS_LIKELY 0
#	define FR_HAS_ULIKELY 0

#	define FR_LIKELY(expr) HEDLEY_STATIC_CAST(bool, (expr))
#	define FR_UNLIKELY(expr) HEDLEY_STATIC_CAST(bool, (expr))
#endif

/// @warning Beware of unexpected optimizations!
#define FR_ASSUME_UNCHECKED(expr) HEDLEY_ASSUME(expr)

/// @warning Beware of unexpected optimizations!
#define FR_UNREACHABLE_UNCHECKED() HEDLEY_UNREACHABLE()

// Sanitizer attributes
// --------------------

#if FR_COMP_GCC_EMULATED && FR_HAS_ATTRIBUTE(no_sanitize)
#	define FR_HAS_NO_SANITIZE_ADDRESS 1
#	define FR_NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))

#	define FR_HAS_NO_SANITIZE_MEMORY 1
#	define FR_NO_SANITIZE_MEMORY __attribute__((no_sanitize("memory")))

#	define FR_HAS_NO_SANITIZE_UNREACHABLE_ 1
#	define FR_NO_SANITIZE_UNREACHABLE __attribute__((no_sanitize("unreachable")))
#else
#	define FR_HAS_NO_SANITIZE_ADDRESS 0
#	define FR_NO_SANITIZE_ADDRESS

#	define FR_HAS_NO_SANITIZE_MEMORY 0
#	define FR_NO_SANITIZE_MEMORY

#	define FR_HAS_NO_SANITIZE_UNREACHABLE_ 0
#	define FR_NO_SANITIZE_UNREACHABLE
#endif

// TARGET_POPCNT
#if defined(__x86_64) && FR_HAS_ATTRIBUTE(target)
#	define FR_HAS_USE_TARGET_POPCNT 1
#	define FR_USE_TARGET_POPCNT __attribute__((target("popcnt")))
#else
#	define FR_HAS_USE_TARGET_POPCNT 0
#	define FR_USE_TARGET_POPCNT
#endif

// Builtins
// --------

#if HEDLEY_HAS_BUILTIN(__builtin_return_address)
#	define FR_GET_CALLER_PC() __builtin_return_address(0)
#	define CATFUZZ_HAS_GET_CALLER_PC 1
#elif FR_COMP_MSVC
extern "C" void* _ReturnAddress();
#	pragma intrinsic(_ReturnAddress);
#	define FR_GET_CALLER_PC() _ReturnAddress()
#	define CATFUZZ_HAS_GET_CALLER_PC 1
#else
#	define FR_GET_CALLER_PC() 0
#	define CATFUZZ_HAS_GET_CALLER_PC 0
#endif

// Diagnostics/warnings
// --------------------

#define FR_HAS_WARNING(warning) HEDLEY_HAS_WARNING(warning)

#define FR_DIAGNOSTIC_PUSH HEDLEY_DIAGNOSTIC_PUSH
#define FR_DIAGNOSTIC_POP HEDLEY_DIAGNOSTIC_POP

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wmissing-field-initializers")
#	define FR_HAS_DIAGNOSTIC_MISSING_FIELD_INITIALIZERS 1
#	define FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS \
		_Pragma("clang diagnostic error \"-Wmissing-field-initializers\"")
#elif FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_MISSING_FIELD_INITIALIZERS 1
#	define FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS \
		_Pragma("GCC diagnostic error \"-Wmissing-field-initializers\"")
#else
#	define FR_HAS_DIAGNOSTIC_MISSING_FIELD_INITIALIZERS 0
#	define FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wconversion")
#	define FR_HAS_DIAGNOSTIC_CONVERSION 1
#	define FR_DIAGNOSTIC_DISABLE_CONVERSION _Pragma("clang diagnostic ignored \"-Wconversion\"")
#elif FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_CONVERSION 1
#	define FR_DIAGNOSTIC_DISABLE_CONVERSION _Pragma("GCC diagnostic ignored \"-Wconversion\"")
#elif FR_COMP_MSVC
#	define FR_HAS_DIAGNOSTIC_CONVERSION 1
#	define FR_DIAGNOSTIC_DISABLE_CONVERSION __pragma(warning(disable:4242 4244 4365 4388 4389))
#else
#	define FR_HAS_DIAGNOSTIC_CONVERSION 0
#	define FR_DIAGNOSTIC_DISABLE_CONVERSION
#endif

#if FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_NRVO 1
#	define FR_DIAGNOSTIC_DISABLE_NRVO _Pragma("GCC diagnostic ignored \"-Wnrvo\"")
#else
#	define FR_HAS_DIAGNOSTIC_NRVO 0
#	define FR_DIAGNOSTIC_DISABLE_NRVO
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wpedantic")
#	define FR_HAS_DIAGNOSTIC_PEDANTIC 1
#	define FR_DIAGNOSTIC_DISABLE_PEDANTIC _Pragma("clang diagnostic ignored \"-Wpedantic\"")
#elif FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_PEDANTIC 1
#	define FR_DIAGNOSTIC_DISABLE_PEDANTIC _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#else
#	define FR_HAS_DIAGNOSTIC_PEDANTIC 0
#	define FR_DIAGNOSTIC_DISABLE_PEDANTIC
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wsign-conversion")
#	define FR_HAS_DIAGNOSTIC_SIGN_CONVERSION 1
#	define FR_DIAGNOSTIC_DISABLE_SIGN_CONVERSION \
		_Pragma("clang diagnostic ignored \"-Wsign-conversion\"")
#elif FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_SIGN_CONVERSION 1
#	define FR_DIAGNOSTIC_DISABLE_SIGN_CONVERSION \
		_Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")
#elif FR_COMP_MSVC
#	define FR_HAS_DIAGNOSTIC_SIGN_CONVERSION 1
#	define FR_DIAGNOSTIC_DISABLE_SIGN_CONVERSION __pragma(warning(disable:4365 4388 4389))
#else
#	define FR_HAS_DIAGNOSTIC_SIGN_CONVERSION 0
#	define FR_DIAGNOSTIC_DISABLE_SIGN_CONVERSION
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wold-style-cast")
#	define FR_HAS_DIAGNOSTIC_OLD_STYLE_CAST 1
#	define FR_DIAGNOSTIC_DISABLE_OLD_STYLE_CAST \
		_Pragma("clang diagnostic ignored \"-Wold-style-cast\"")
#elif FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_OLD_STYLE_CAST 1
#	define FR_DIAGNOSTIC_DISABLE_OLD_STYLE_CAST \
		_Pragma("GCC diagnostic ignored \"-Wold-style-cast\"")
#else
#	define FR_HAS_DIAGNOSTIC_OLD_STYLE_CAST 0
#	define FR_DIAGNOSTIC_DISABLE_OLD_STYLE_CAST
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wself-assign-overloaded")
#	define FR_HAS_DIAGNOSTIC_SELF_ASSIGN 1
#	define FR_DIAGNOSTIC_DISABLE_SELF_ASSIGN \
		_Pragma("clang diagnostic ignored \"-Wself-assign-overloaded\"")
#else
#	define FR_HAS_DIAGNOSTIC_SELF_ASSIGN 0
#	define FR_DIAGNOSTIC_DISABLE_SELF_ASSIGN
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wself-move")
#	define FR_HAS_DIAGNOSTIC_SELF_MOVE 1
#	define FR_DIAGNOSTIC_DISABLE_SELF_MOVE \
		_Pragma("clang diagnostic ignored \"-Wself-move\"")
#else
#	define FR_HAS_DIAGNOSTIC_SELF_MOVE 0
#	define FR_DIAGNOSTIC_DISABLE_SELF_MOVE
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wreserved-identifier")
#	define FR_HAS_DIAGNOSTIC_RESERVED_IDENTIFIER 1
#	define FR_DIAGNOSTIC_DISABLE_RESERVED_IDENTIFIER \
		_Pragma("clang diagnostic ignored \"-Wreserved-identifier\"")
#else
#	define FR_HAS_DIAGNOSTIC_RESERVED_IDENTIFIER 0
#	define FR_DIAGNOSTIC_DISABLE_RESERVED_IDENTIFIER
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wredundant-consteval-if")
#	define FR_HAS_DIAGNOSTIC_REDUNDANT_CONSTEVAL_IF 1
#	define FR_DIAGNOSTIC_DISABLE_REDUNDANT_CONSTEVAL_IF \
		_Pragma("clang diagnostic ignored \"-Wredundant-consteval-if\"")
#else
#	define FR_HAS_DIAGNOSTIC_REDUNDANT_CONSTEVAL_IF 0
#	define FR_DIAGNOSTIC_DISABLE_REDUNDANT_CONSTEVAL_IF
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wunused-function")
#	define FR_HAS_DIAGNOSTIC_UNUSED_FUNCTION 1
#	define FR_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION \
		_Pragma("clang diagnostic ignored \"-Wunused-function\"")
#elif FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_UNUSED_FUNCTION 1
#	define FR_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION \
		_Pragma("GCC diagnostic ignored \"-Wunused-function\"")
#else
#	define FR_HAS_DIAGNOSTIC_UNUSED_FUNCTION 0
#	define FR_DIAGNOSTIC_DISABLE_UNUSED_FUNCTION
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wunneeded-internal-declaration")
#	define FR_HAS_DIAGNOSTIC_UNNEEDED_INTERNAL 1
#	define FR_DIAGNOSTIC_DISABLE_UNNEEDED_INTERNAL
		_Pragma("clang diagnostic ignored \"-Wunneeded-internal-declaration\"")
#else
#	define FR_HAS_DIAGNOSTIC_UNNEEDED_INTERNAL 0
#	define FR_DIAGNOSTIC_DISABLE_UNNEEDED_INTERNAL
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wundefined-var-template")
#	define FR_HAS_DIAGNOSTIC_UNDEFINED_VAR_TEMPLATE 1
#	define FR_DIAGNOSTIC_DISABLE_UNDEFINED_VAR_TEMPLATE
		_Pragma("clang diagnostic ignored \"-Wundefined-var-template\"")
#else
#	define FR_HAS_DIAGNOSTIC_UNDEFINED_VAR_TEMPLATE 0
#	define FR_DIAGNOSTIC_DISABLE_UNDEFINED_VAR_TEMPLATE
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wundefined-internal")
#	define FR_HAS_DIAGNOSTIC_UNDEFINED_INTERNAL 1
#	define FR_DIAGNOSTIC_DISABLE_UNDEFINED_INTERNAL
		_Pragma("clang diagnostic ignored \"-Wundefined-internal\"")
#else
#	define FR_HAS_DIAGNOSTIC_UNDEFINED_INTERNAL 0
#	define FR_DIAGNOSTIC_DISABLE_UNDEFINED_INTERNAL
#endif

#if FR_COMP_CLANG && FR_HAS_WARNING("-Wunreachable-code")
#	define FR_HAS_DIAGNOSTIC_UNREACHABLE_CODE 1
#	define FR_DIAGNOSTIC_DISABLE_UNREACHABLE_CODE \
		_Pragma("clang diagnostic ignored \"-Wunreachable-code\"")
#elif FR_COMP_GCC
#	define FR_HAS_DIAGNOSTIC_UNREACHABLE_CODE 1
#	define FR_DIAGNOSTIC_DISABLE_UNREACHABLE_CODE \
		_Pragma("GCC diagnostic ignored \"-Wunreachable-code\"")
#else
#	define FR_HAS_DIAGNOSTIC_UNREACHABLE_CODE 0
#	define FR_DIAGNOSTIC_DISABLE_UNREACHABLE_CODE
#endif

// Misc
// ----

#if FR_COMP_CLANG || FR_COMP_GCC
#	define FR_HAS_FLATTEN 1
#	define FR_FLATTEN __attribute__((flatten))
#else
#	define FR_FLATTEN
#	define FR_HAS_FLATTEN 0
#endif

// MSVC ignores [[no_unique_address]] by default for ABI reasons
#if HEDLEY_MSVC_VERSION_CHECK(19, 28, 0)
#	define FR_HAS_NO_UNIQUE_ADDRESS 1
#	define FR_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#	define FR_HAS_NO_UNIQUE_ADDRESS 1
#	define FR_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if (__cpp_structured_bindings >= 202601L)
#	define FR_HAS_PACK_BINDINGS 1
#else
#	define FR_HAS_PACK_BINDINGS 0
#endif

#ifdef NDEBUG
#	define FR_IS_DEBUG_BUILD 0
#else
#	define FR_IS_DEBUG_BUILD 1
#endif

#endif // include guard
