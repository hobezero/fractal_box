cmake_minimum_required(VERSION 4.2)

add_library(glad STATIC)
add_library(glad::glad ALIAS glad)

target_include_directories(glad
	SYSTEM INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE
		include
)

target_link_libraries(glad PUBLIC
	fractal_box::default_compile_options
)

target_sources(glad
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS include
		FILES
			include/KHR/khrplatform.h
			include/glad/glad.h
	PRIVATE
		src/glad.c
)
