cmake_minimum_required(VERSION 3.28)

project(fractal_box
	VERSION 0.1.0
	DESCRIPTION "An experimental game based on orbital mechanics"
	LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
message(STATUS "FractalBox: Predefined module path: '${CMAKE_MODULE_PATH}'")

# Options
# -------

option(FR_INSTALL "FractalBox: Enable install target" ON)
option(FR_BUILD_TESTING "FractalBox: Build unit test targets" OFF)
option(FR_BUILD_BENCH "FractalBox: Build benchmarking targets" OFF)
option(FR_DEV_MODE
	"FractalBox: Build with a developer-focused set of compile options and definitions" ON)

set(FR_OVERRIDE_ASSERT_LEVEL "AUTO" CACHE STRING "Override FR_ASSERT_LEVEL")
set(valid_assert_levels AUTO NONE FAST DEFAULT AUDIT MAX)
set_property(CACHE FR_OVERRIDE_ASSERT_LEVEL PROPERTY STRINGS ${valid_assert_levels})

set(FR_OVERRIDE_LOG_LEVEL "AUTO" CACHE STRING "Override FR_LOG_LEVEL")
set(valid_log_levels AUTO NONE FATAL ERROR WARN INFO DEBUG TRACE MAX)
set_property(CACHE FR_OVERRIDE_LOG_LEVEL PROPERTY STRINGS ${valid_log_levels})

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

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(FR_DEV_MODE)
	message(STATUS "FractalBox: Developer mode configuration enabled")
endif()

include(cmake/CompilerFlags.cmake)

if(FR_FORCE_COLORED_OUTPUT)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-fdiagnostics-color=always)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
		add_compile_options(-fcolor-diagnostics)
	endif()
endif()

if(NOT "${FR_OVERRIDE_ASSERT_LEVEL}" IN_LIST valid_assert_levels)
	message(SEND_ERROR
		"FractalBox: Invalid FR_OVERRIDE_ASSERT_LEVEL. Acceptable values: ${valid_assert_levels}")
endif()

if(NOT "${FR_OVERRIDE_LOG_LEVEL}" IN_LIST valid_log_levels)
	message(SEND_ERROR "FractalBox: Invalid FR_OVERRIDE_LOG_LEVEL '${FR_OVERRIDE_LOG_LEVEL}'."
		"Acceptable values: ${valid_log_levels}")
endif()

# Sanitizers
# ----------

include(cmake/Sanitizers.cmake)
enable_sanitizer_options("FR" "FractalBox" fractal_box)

# Bundled dependencies
# --------------------

add_subdirectory(deps/glad)
add_subdirectory(deps/imgui_backends)

# Subprojects
# -----------

add_subdirectory(engine)
add_subdirectory(demos)

# Installation
# ------------

if(FR_INSTALL)
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND FR_DEV_MODE)
		set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${PROJECT_SOURCE_DIR}/.install")
		message(STATUS "FractalBox: Set install prefix to local directory")
	endif()
	message(STATUS "FractalBox: CMAKE_INSTALL_PREFIX: '${CMAKE_INSTALL_PREFIX}'")
endif()
