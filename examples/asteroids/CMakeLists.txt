add_executable(asteroids WIN32)

target_link_libraries(asteroids PRIVATE
	$<$<BOOL:${FR_DEV_MODE}>:fractal_box::default_warnings>
	$<$<BOOL:${FR_DEV_MODE}>:fractal_box::default_errors>
	fractal_box::sanitizer_options
	fractal_box::engine
	asteroids::assets
)

target_sources(asteroids
	PRIVATE
		FILE_SET HEADERS
		BASE_DIRS src
)

if(FR_INSTALL)
	include(GNUInstallDirs)
	install(TARGETS asteroids DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

add_subdirectory(src/asteroids)
add_subdirectory(assets)
