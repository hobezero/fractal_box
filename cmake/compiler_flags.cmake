include_guard(GLOBAL)

# Profilers
# ---------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
	(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
)
	add_library(fractal_box_gprof_flags INTERFACE)
	add_library(fractal_box::gprof_flags ALIAS fractal_box_gprof_flags)

	target_compile_options(fractal_box_gprof_flags INTERFACE -pg)
	target_link_options(fractal_box_gprof_flags INTERFACE -pg)
endif()

# Warnings
# --------

set(msvc_warnings
	# VS documentation at https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-c4000-c5999?view=msvc-170
	# TODO: Consider https://github.com/friendlyanon/cmake-init/blob/b618b72ec08802636cbaf4cc7fb5e4327906581c/cmake-init/templates/common/CMakePresets.json#L97-L98
	# TODO: Consider https://learn.microsoft.com/en-us/cpp/build/reference/zc-conformance?view=msvc-170

	# Baseline reasonable warnings
	/W4
	# "conversion from 'type1' to 'type1', possible loss of data"
	/w14242
	# "conversion from 'type1:field_bits' to 'type2:field_bits', possible"
	# loss of data
	/w14254
	# "member function does not override any base class virtual member function"
	/w14263
	# "class has virtual functions, but destructor is not virtual. Instances of
	# this class may not be destructed correctly"
	/w14265
	# "unsigned/negative constant mismatch"
	/w14287
	# nonstandard extension used: 'variable': loop control variable declared in the
	# for-loop is used outside the for-loop scope
	/we4289
	# "expression is always 'boolean_value'"
	/w14296
	# "pointer truncation from 'type1' to 'type2'"
	/w14311
	# "expression before comma evaluates to a function which is missing an argument list"
	/w14545
	# "function call before comma missing argument list"
	/w14546
	# "operator before comma has no effect; expected operator with side-effect"
	/w14547
	# "operator before comma has no effect; did you intend 'operator'?"
	/w14549
	# "expression has no effect; expected expression with side- effect"
	/w14555
	# "pragma warning: there is no warning number 'number'"
	/w14619
	# Enable warning on thread un-safe static member initialization
	/w14640
	# "Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected
	# runtime behavior"
	/w14826
	# "wide string literal cast to 'LPSTR'"
	/w14905
	# "string literal cast to 'LPWSTR'"
	/w14906
	# "Illegal copy-initialization; more than one user-defined conversion has been implicitly applied"
	/w14928
	# "conversion from 'type1' to 'type2', possible loss of data""
	/w14242
	# "'HRESULT' is being converted to 'bool'; are you sure this is what you want?"
	/w14165
	# Warn on undefined preprocessor macros. Similar to GCC's `-Wundef`
	/w14668
	# Standards conformance mode for MSVC compiler
	/permissive-
)

set(clang_gcc_warnings
	# Not *all* warnings actually, just uncontroversial ones
	-Wall
	# Reasonable and standard
	-Wextra
	# Don't warn if designated initializers skip a member
	-Wno-missing-field-initializers
	# Warn the user if a variable declaration shadows one from a parent context
	-Wshadow
	# C++ only. Warn the user if a class with virtual functions has a non-virtual destructor
	$<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
	# C++ only. Warn on trying to call a non-virtual destructor on an abstract class
	$<$<COMPILE_LANGUAGE:CXX>:-Wdelete-non-virtual-dtor>
	# C++ only. Warn for C-style casts
	$<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
	# Warn whenever a pointer is cast such that the required alignment of the target is increased
	-Wcast-align
	# Warn on anything being unused
	-Wunused
	# C++ only. Warn if you overload (not override) a virtual function
	$<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
	# Warn if non-standard C or C++ is used
	-Wpedantic
	# Warn on type conversions that may lose data
	-Wconversion
	# Warn on sign conversions
	-Wsign-conversion
	# Warn if a null dereference is detected
	-Wnull-dereference
	# Warn if a `float` is implicitly promoted to `double`
	-Wdouble-promotion
	# Warn on security issues around functions that format output (i.e. `printf`)
	-Wformat=2
	# Warn if a global function is defined without a previous prototype declaration
	$<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>
	# Warn if a function is declared or defined without specifying the argument types
	$<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
	# Warn if an undefined identifier is evaluated in an #if directive.
	# Such identifiers are replaced with zero
	-Wundef
	# Warn whenever a pointer is cast so as to remove a type qualifier from the target type.
	# For example, warn if a `const char*` is cast to an ordinary `char*`
	-Wcast-qual
	# C only. Give string constants the type `const char[length]`
	$<$<COMPILE_LANGUAGE:C>:-Wwrite-strings>
	# Warn when performing CTAD on a type with no explicitly written deduction guides
	-Wctad-maybe-unsupported
)

set(clang_warnings
	${clang_gcc_warnings}
	# Warn about code that can't be executed
	-Wunreachable-code
	# Warn when a switch case falls through
	-Wimplicit-fallthrough
	# Warn on redundant semicolons
	-Wextra-semi
)

set(gcc_warnings
	${clang_gcc_warnings}
	# Warn if indentation implies blocks where blocks do not exist
	-Wmisleading-indentation
	# Warn if an `if` / `else` chain has duplicated conditions
	-Wduplicated-cond
	# Warn if an `if` / `else` branches have duplicated code
	-Wduplicated-branches
	# Warn about logical operations being used where bitwise were probably wanted
	-Wlogical-op
	# Warn if you perform a cast to the same type. Disabled for now
#	$<$<COMPILE_LANGUAGE:CXX>:-Wuseless-cast>
	# Warn if a goto statement or a switch statement jumps forward across the initialization of
	# a variable
	$<$<COMPILE_LANGUAGE:C>:-Wjump-misses-init>
	# Warn if a user-supplied include directory does not exist
	-Wmissing-include-dirs
	# Warn if a precompiled header is found in the search path but cannot be used
	-Winvalid-pch
	# Warn when a switch case falls through. Do not recognize `FALLTHRU` comments
	-Wimplicit-fallthrough=5
	# Warn if the compiler can't apply NRVO in a context where it is allowed by [class.copy.elision]
	$<$<COMPILE_LANGUAGE:CXX>:-Wnrvo>
	# DON'T warn on use of `std::hardware_*_interference_size`. GCC is concerned too much about ABI
	# stability
	-Wno-interference-size
	# DON'T warn on potential null dereference until we figure out  how to fix spurious warnings
	# in the standard library
	-Wno-null-dereference
)

if(MSVC)
	set(project_warnings ${msvc_warnings})
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
	set(project_warnings ${clang_warnings})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(project_warnings ${gcc_warnings})
else()
	message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
endif()

add_library(fractal_box_default_warnings INTERFACE)
add_library(fractal_box::default_warnings ALIAS fractal_box_default_warnings)
target_compile_options(fractal_box_default_warnings INTERFACE ${project_warnings})

# Granular warning targets
# ------------------------

add_library(fractal_box_no_deprecated_warnings INTERFACE)
add_library(fractal_box::no_deprecated_warnings ALIAS fractal_box_no_deprecated_warnings)

if(CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
	target_compile_options(fractal_box_no_deprecated_warnings INTERFACE "/wd4996")
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(fractal_box_no_deprecated_warnings INTERFACE "-Wno-deprecated-declarations")
endif()

# Errors
# ------

set(msvc_errors
	# Error on missing return in a non-void function. Enabled by Microsoft by default
	/we4716
	# "nonstandard extension used: 'variable': loop control variable declared in the
	# for-loop is used outside the for-loop scope"
	/we4289
)

set(clang_gcc_errors
	# Error on missing return in a non-void function
	-Werror=return-type
	#  Error whenever a switch lacks a case for a named code of the enumeration
	-Werror=switch
	# Error when a switch case falls through. Use `[[fallthrough]]` to fix
	-Werror=implicit-fallthrough
)

set(clang_errors
	${clang_gcc_errors}
	-Werror=reserved-identifier
)

set(gcc_errors
	${clang_gcc_errors}
	# Give an error whenever the base standard requires a diagnostic.
	# NOTE: not the same as `-Werror=pedantic`
	-pedantic-errors
)

if(MSVC)
	set(project_errors ${msvc_errors})
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
	set(project_errors ${clang_errors})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(project_errors ${gcc_errors})
else()
	message(AUTHOR_WARNING "No compiler errors set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
endif()

add_library(fractal_box_default_errors INTERFACE)
add_library(fractal_box::default_errors ALIAS fractal_box_default_errors)
target_compile_options(fractal_box_default_errors INTERFACE ${project_errors})

# Compile options
# ---------------

add_library(fractal_box_default_compile_options INTERFACE)
add_library(fractal_box::default_compile_options ALIAS fractal_box_default_compile_options)

set(msvc_defines
	# Do not warn on "unsafe" C functions
	_CRT_SECURE_NO_WARNINGS
	# Remove `min`/`max` macros from `Windef.h` (and `Windows.h`)
	NOMINMAX
	# Do not include unnecessary WinAPI headers
	WIN32_LEAN_AND_MEAN
)

set(msvc_compile_options
	# Set source and execution character sets to UTF-8
	/utf-8
	# Specify strict standard conformance mode
	/permissive-
	# Use the standard conforming preprocessor
	/Zc:preprocessor
	# Enable standard C++ stack unwinding
	$<$<COMPILE_LANGUAGE:CXX>:/EHsc>
	# Skip checks for a null pointer return of `operator new`
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:throwingNew>
	# Select strict volatile semantics as defined by the C++ standard.
	# Acquire/release semantics are not guaranteed on volatile accesses
	/volatile:iso
	# Assume that `operator new` can throw
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:throwingNew>
	# Enable the `__cplusplus` macro to report the supported standard
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
	# Enable external linkage for constexpr variables
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:externConstexpr>
	# Cnables checks for behavior around shadowing of template parameters
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:templateScope>
	# Enable C++ conforming enum underlying type and enumerator type deduction
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:enumTypes>
	# Remove unreferenced data or functions that are COMDATs, or that only have internal linkage
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:inline>
	# allow external linkage for constexpr variables
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:externConstexpr>
	# Enable conforming lambda grammar and processing support
	$<$<COMPILE_LANGUAGE:CXX>:/Zc:lambda>
)

set(clang_gcc_compile_options
	-fno-omit-frame-pointer
	-fvisibility=hidden
	$<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>
)

if(MSVC)
	target_compile_definitions(fractal_box_default_compile_options INTERFACE ${msvc_defines})
	target_compile_options(fractal_box_default_compile_options INTERFACE ${msvc_compile_options})
elseif(compiler_id STREQUAL "GNU" OR (compiler_id MATCHES ".*Clang"))
	# NOTE: This branch doesn't apply to clang-cl
	target_compile_options(fractal_box_default_compile_options INTERFACE ${clang_gcc_compile_options})
endif()

# Wrappers
# --------

# Create a new target that is equivalent to the given target except warnings in header files are
# ignored. Works on GCC and Clang
# TODO: Implement MSVC version by passing /external:... flags or wait untill CMake supports it
function(dewarnify target new_target new_alias)
	add_library(${new_target} INTERFACE)
	add_library(${new_alias} ALIAS ${new_target})
	target_link_libraries(${new_target} INTERFACE ${target})
	target_include_directories(${new_target} SYSTEM INTERFACE
		$<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>
	)
endfunction()
