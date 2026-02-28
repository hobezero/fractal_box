add_executable(fractal_box_tests)

target_include_directories(fractal_box_tests PRIVATE src)

target_link_libraries(fractal_box_tests PRIVATE
	$<$<BOOL:${FR_DEV_MODE}>:fractal_box::default_warnings>
	$<$<BOOL:${FR_DEV_MODE}>:fractal_box::default_errors>
	fractal_box::sanitizer_options
	fractal_box::engine
	fractal_box::wrapped_catch2_with_main
)

add_test(NAME test_assert COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][assert])
add_test(NAME test_collision COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][collision])
add_test(NAME test_components COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][components])
add_test(NAME test_concepts COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][concepts])
add_test(NAME test_containers COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][containers])
add_test(NAME test_error_handling COMMAND $<TARGET_FILE:fractal_box_tests>
	[u][engine][error_handling])
add_test(NAME test_functional COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][functional])
add_test(NAME test_hashing COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][hashing])
add_test(NAME test_input COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][input])
add_test(NAME test_math COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][math])
add_test(NAME test_message COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][message])
add_test(NAME test_meta COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][meta])
add_test(NAME test_poly COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][poly])
add_test(NAME test_preprocessor COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][preprocessor])
add_test(NAME test_ref COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][ref])
add_test(NAME test_reflection COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][reflection])
add_test(NAME test_scope COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][scope])
add_test(NAME test_type_index COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][type_index])
add_test(NAME test_utils COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][utils])
add_test(NAME test_world COMMAND $<TARGET_FILE:fractal_box_tests> [u][engine][world])

add_subdirectory(src)
