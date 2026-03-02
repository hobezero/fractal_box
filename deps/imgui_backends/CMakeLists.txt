cmake_minimum_required(VERSION 4.2)

find_package(SDL2 2.30 REQUIRED)
find_package(imgui REQUIRED) # TODO: Specify version once we move back to Conan

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

add_library(imgui_backends_sdl2 STATIC)
add_library(imgui_backends::sdl2 ALIAS imgui_backends_sdl2)

target_link_libraries(imgui_backends_sdl2 PUBLIC
	imgui_backends::common
	SDL2::SDL2
)

target_include_directories(imgui_backends_sdl2 SYSTEM INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

target_sources(imgui_backends_sdl2 PRIVATE
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS include
		FILES
			include/imgui_backends/imgui_impl_sdl2.h
	PRIVATE
		src/imgui_backends/imgui_impl_sdl2.cpp
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
