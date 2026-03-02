cmake_minimum_required(VERSION 4.2)

project(fractal_box
	VERSION 0.1.0
	DESCRIPTION "An experimental game engine for large scale simulations"
	LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
message(STATUS "FractalBox: Predefined module path: '${CMAKE_MODULE_PATH}'")

# Options
# -------

option(FR_INSTALL "FractalBox: Enable install target" ON)
option(FR_BUILD_TESTING "FractalBox: Build unit test targets" ${PROJECT_IS_TOP_LEVEL})
option(FR_BUILD_BENCH "FractalBox: Build benchmarking targets" ${PROJECT_IS_TOP_LEVEL})
option(
	FR_DEV_MODE
	"FractalBox: Build with a developer-focused set of compile options and definitions"
	${PROJECT_IS_TOP_LEVEL}
)

set(FR_OVERRIDE_ASSERT_LEVEL "AUTO" CACHE STRING "Override FR_ASSERT_LEVEL")
set(valid_assert_levels AUTO NONE FAST DEFAULT AUDIT MAX)
set_property(CACHE FR_OVERRIDE_ASSERT_LEVEL PROPERTY STRINGS ${valid_assert_levels})
if(NOT "${FR_OVERRIDE_ASSERT_LEVEL}" IN_LIST valid_assert_levels)
	message(SEND_ERROR
		"FractalBox: Invalid FR_OVERRIDE_ASSERT_LEVEL. Acceptable values: ${valid_assert_levels}")
endif()

set(FR_OVERRIDE_LOG_LEVEL "AUTO" CACHE STRING "Override FR_LOG_LEVEL")
set(valid_log_levels AUTO NONE FATAL ERROR WARN INFO DEBUG TRACE MAX)
set_property(CACHE FR_OVERRIDE_LOG_LEVEL PROPERTY STRINGS ${valid_log_levels})
if(NOT "${FR_OVERRIDE_LOG_LEVEL}" IN_LIST valid_log_levels)
	message(SEND_ERROR "FractalBox: Invalid FR_OVERRIDE_LOG_LEVEL '${FR_OVERRIDE_LOG_LEVEL}'."
		"Acceptable values: ${valid_log_levels}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
	(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
)
	option(FR_ENABLE_GPROF "FractalBox: Enable gprof instrumentation support" OFF)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
	(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
)
	# For pretty output to the terminal
	option(FR_FORCE_COLORED_OUTPUT "FractalBox: Force compiler to produce ANSI-colored output" OFF)
endif()

# Global CMake configuration
# --------------------------

set(CMAKE_CXX_SCAN_FOR_MODULES OFF) # Do not assume that we use modules

# IDE & tooling setup
# -------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Make the executable a default target to build & run in Visual Studio
# set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT MY_EXE_TARGET)

# Compiler flags
# --------------

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
elseif(CMAKE_CXX_STANDARD LESS 23)
	message(FATAL_ERROR "FractalBox requires at least C++23")
endif()

if(FR_DEV_MODE)
	message(STATUS "FractalBox: Developer mode configuration enabled")
endif()

include(cmake/compiler_flags.cmake)

if(FR_FORCE_COLORED_OUTPUT)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-fdiagnostics-color=always)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
		add_compile_options(-fcolor-diagnostics)
	endif()
endif()


# Sanitizers
# ----------

include(cmake/sanitizers.cmake)
enable_sanitizer_options("FR" "FractalBox" fractal_box)

# Bundled dependencies
# --------------------

add_subdirectory(deps/glad)
add_subdirectory(deps/imgui_backends)

# Subprojects
# -----------

add_subdirectory(engine)
add_subdirectory(examples)

# Testing
# -------

if(FR_BUILD_TESTING OR FR_BUILD_BENCH)
	# NOTE: include(CTest) also provides the BUILD_TESTING option
	if(PROJECT_IS_TOP_LEVEL)
		include(CTest)
	endif()

	find_package(Catch2 3.7 REQUIRED)

	add_library(fractal_box_wrapped_catch2_with_main INTERFACE)
	add_library(fractal_box::wrapped_catch2_with_main ALIAS fractal_box_wrapped_catch2_with_main)

	target_link_libraries(fractal_box_wrapped_catch2_with_main INTERFACE Catch2::Catch2WithMain)
	target_compile_definitions(fractal_box_wrapped_catch2_with_main INTERFACE
		CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
	)
endif()

# Installation
# ------------

if(FR_INSTALL)
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND FR_DEV_MODE)
		set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${PROJECT_SOURCE_DIR}/.install")
		message(STATUS "FractalBox: Set install prefix to local directory")
	endif()
	message(STATUS "FractalBox: CMAKE_INSTALL_PREFIX: '${CMAKE_INSTALL_PREFIX}'")

	include(GNUInstallDirs)
	include(CMakePackageConfigHelpers)

	install(
		TARGETS
			fractal_box_engine
			fractal_box_assets
			fractal_box_sanitizer_options
			fractal_box_default_compile_options
			fractal_box_wrapped_glm
			imgui_backends_opengl3
			imgui_backends_common
			imgui_backends_sdl2
			cmrc-base
			glad
		EXPORT fractal_box_targets
		FILE_SET HEADERS
	)

	install(
		EXPORT fractal_box_targets
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fractal_box"
		NAMESPACE fractal_box::
	)

	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/fractal_box-config-version.cmake"
		VERSION ${PROJEC_VERSION}
		COMPATIBILITY ExactVersion
	)
	configure_package_config_file(
		"${PROJECT_SOURCE_DIR}/cmake/fractal_box-config.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/fractal_box-config.cmake"
		INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fractal_box"
		PATH_VARS PROJECT_NAME PROJECT_VERSION
	)
	install(
		FILES
			"${CMAKE_CURRENT_BINARY_DIR}/fractal_box-config-version.cmake"
			"${CMAKE_CURRENT_BINARY_DIR}/fractal_box-config.cmake"
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fractal_box"
	)
endif()
