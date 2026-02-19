cmake_minimum_required(VERSION 3.29)

find_package(Threads REQUIRED)
find_package(hedley 15 REQUIRED)
find_package(fmt 11 REQUIRED)
find_package(glm 1 REQUIRED)
find_package(cityhash REQUIRED)
find_package(SDL2 2.0.18 REQUIRED)
find_package(stb REQUIRED)
find_package(imgui REQUIRED) # TODO: Specify version once we move back to Conan

add_library(fractal_box_wrapped_glm INTERFACE)
add_library(fractal_box::wrapped_glm ALIAS fractal_box_wrapped_glm)
target_link_libraries(fractal_box_wrapped_glm INTERFACE glm::glm)
target_compile_definitions(fractal_box_wrapped_glm INTERFACE GLM_ENABLE_EXPERIMENTAL)

add_library(fractal_box_engine STATIC)
add_library(fractal_box::engine ALIAS fractal_box_engine)

target_include_directories(fractal_box_engine
	PUBLIC include
	PRIVATE src
)

target_link_libraries(fractal_box_engine
	PUBLIC
		fractal_box::assets
		imgui_backends::sdl2
		imgui_backends::opengl3
		imgui::imgui
		glad::glad
		cmrc::base
		stb::stb
		SDL2::SDL2
		cityhash::cityhash
		fractal_box::wrapped_glm
		fmt::fmt-header-only
		hedley::hedley
		Threads::Threads
		fractal_box::default_compile_options
	PRIVATE
		fractal_box::sanitizer_options
		$<$<BOOL:${FR_DEV_MODE}>:fractal_box::default_warnings>
		$<$<BOOL:${FR_DEV_MODE}>:fractal_box::default_errors>
)

if(NOT FR_OVERRIDE_ASSERT_LEVEL EQUAL "AUTO")
	# TODO: Support multi-config builds
	target_compile_definitions(fractal_box_engine PUBLIC
		FR_OVERRIDE_ASSERT_LEVEL=FR_ASSERT_LEVEL_${FR_OVERRIDE_ASSERT_LEVEL}
	)
endif()

if(NOT FR_OVERRIDE_LOG_LEVEL EQUAL "AUTO")
	# TODO: Support multi-config builds
	target_compile_definitions(fractal_box_engine PUBLIC
		FR_OVERRIDE_LOG_LEVEL=FR_LOG_LEVEL_${FR_OVERRIDE_LOG_LEVEL}
	)
endif()

if(FR_ENABLE_GPROF)
	target_link_libraries(fractal_box_engine PUBLIC fractal_box::gprof_flags)
endif()

add_subdirectory(include/fractal_box)
add_subdirectory(src/fractal_box)
add_subdirectory(assets)

# Testing
# -------

if(FR_BUILD_TESTING OR FR_BUILD_BENCH)
	# Include CTest only if we are in the root directory
	# NOTE: include(CTest) also provides the BUILD_TESTING option
	if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
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

if(FR_BUILD_TESTING AND BUILD_TESTING)
	message(STATUS "FractalBox: Unit tests enabled")
	add_subdirectory(test)
endif()

if(FR_BUILD_BENCH AND BUILD_TESTING)
	message(STATUS "FractalBox: Benchmarks enabled")
	find_package(nanobench 4.3 REQUIRED)
	add_subdirectory(bench)
endif()
