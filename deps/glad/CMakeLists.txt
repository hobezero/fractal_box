cmake_minimum_required(VERSION 3.28)

add_library(glad STATIC)
add_library(glad::glad ALIAS glad)

target_include_directories(glad
	SYSTEM INTERFACE
		include
	PRIVATE
		include
)

target_link_libraries(glad PUBLIC
	fractal_box::default_compile_options
)

target_sources(glad PRIVATE
	include/KHR/khrplatform.h
	include/glad/glad.h
	src/glad.c
)
