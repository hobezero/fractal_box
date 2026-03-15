cmake_minimum_required(VERSION 4.2)

find_package(SDL3 3.4 REQUIRED)
find_package(imgui 1.92 REQUIRED)

add_library(imgui_backends_common INTERFACE)
add_library(imgui_backends::common ALIAS imgui_backends_common)

target_link_libraries(imgui_backends_common INTERFACE
	fractal_box::default_compile_options
	imgui::imgui
)

target_include_directories(imgui_backends_common SYSTEM INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

add_library(imgui_backends_sdl3 STATIC)
add_library(imgui_backends::sdl3 ALIAS imgui_backends_sdl3)

target_link_libraries(imgui_backends_sdl3 PUBLIC
	imgui_backends::common
	sdl::sdl
)

target_include_directories(imgui_backends_sdl3 SYSTEM INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

target_sources(imgui_backends_sdl3 PRIVATE
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS include
		FILES
			include/imgui_backends/imgui_impl_sdl3.h
	PRIVATE
		src/imgui_backends/imgui_impl_sdl3.cpp
)

add_library(imgui_backends_opengl3 STATIC)
add_library(imgui_backends::opengl3 ALIAS imgui_backends_opengl3)

target_link_libraries(imgui_backends_opengl3 PUBLIC
	imgui_backends::common
	glad::glad
)

target_include_directories(imgui_backends_opengl3 SYSTEM INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

target_sources(imgui_backends_opengl3 PRIVATE
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS include
		FILES
			include/imgui_backends/imgui_impl_opengl3.h
	PRIVATE
		src/imgui_backends/imgui_impl_opengl3.cpp
)
