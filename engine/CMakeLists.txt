cmake_minimum_required(VERSION 4.2)

add_library(fractal_box_engine STATIC)
add_library(fractal_box::engine ALIAS fractal_box_engine)

find_package(Threads REQUIRED)
find_package(hedley 15 REQUIRED)
find_package(fmt 12.1 REQUIRED)
find_package(sfl 2.0 REQUIRED)
find_package(glm 1.0 REQUIRED)
find_package(cityhash REQUIRED)
find_package(SDL2 2.32 REQUIRED)
find_package(stb REQUIRED)
find_package(imgui 1.91 REQUIRED)

add_library(fractal_box_wrapped_glm INTERFACE)
add_library(fractal_box::wrapped_glm ALIAS fractal_box_wrapped_glm)
target_link_libraries(fractal_box_wrapped_glm INTERFACE glm::glm)
target_compile_definitions(fractal_box_wrapped_glm INTERFACE GLM_ENABLE_EXPERIMENTAL)

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
		sfl::sfl
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
	target_compile_definitions(fractal_box_engine PUBLIC
		FR_OVERRIDE_ASSERT_LEVEL=FR_ASSERT_LEVEL_${FR_OVERRIDE_ASSERT_LEVEL}
	)
endif()

if(NOT FR_OVERRIDE_LOG_LEVEL EQUAL "AUTO")
	target_compile_definitions(fractal_box_engine PUBLIC
		FR_OVERRIDE_LOG_LEVEL=FR_LOG_LEVEL_${FR_OVERRIDE_LOG_LEVEL}
	)
endif()

if(FR_ENABLE_GPROF)
	target_link_libraries(fractal_box_engine PUBLIC fractal_box::gprof_flags)
endif()

target_sources(fractal_box_engine
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS include
	PRIVATE
		FILE_SET private_headers
		TYPE HEADERS
		BASE_DIRS src
)

set_target_properties(fractal_box_engine PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

add_subdirectory(include/fractal_box)
add_subdirectory(src/fractal_box)
add_subdirectory(assets)

# Testing
# -------

if(FR_BUILD_TESTING AND BUILD_TESTING)
	message(STATUS "FractalBox: Unit tests enabled")
	add_subdirectory(test)
endif()

if(FR_BUILD_BENCH AND BUILD_TESTING)
	message(STATUS "FractalBox: Benchmarks enabled")
	find_package(nanobench 4.3 REQUIRED)
	add_subdirectory(bench)
endif()
