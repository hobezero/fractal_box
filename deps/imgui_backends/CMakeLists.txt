cmake_minimum_required(VERSION 3.28)

find_package(SDL2 2.30 REQUIRED)
find_package(imgui REQUIRED) # TODO: Specify version once we move back to Conan

add_library(imgui_backends_common INTERFACE)
add_library(imgui_backends::common ALIAS imgui_backends_common)

target_include_directories(imgui_backends_common SYSTEM INTERFACE
	include
)

target_link_libraries(imgui_backends_common INTERFACE
	fractal_box::default_compile_options
	imgui::imgui
)

add_library(imgui_backends_sdl2 STATIC)
add_library(imgui_backends::sdl2 ALIAS imgui_backends_sdl2)

target_link_libraries(imgui_backends_sdl2 PUBLIC
	imgui_backends::common
	SDL2::SDL2
)

target_sources(imgui_backends_sdl2 PRIVATE
	include/imgui_impl_sdl2.h
	src/imgui_impl_sdl2.cpp
)

add_library(imgui_backends_opengl3 STATIC)
add_library(imgui_backends::opengl3 ALIAS imgui_backends_opengl3)

target_link_libraries(imgui_backends_opengl3 PUBLIC
	imgui_backends::common
	glad::glad
)

target_sources(imgui_backends_opengl3 PRIVATE
	include/imgui_impl_opengl3.h
	src/imgui_impl_opengl3.cpp
)
