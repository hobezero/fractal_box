add_executable(fractal_box_bench)

target_link_libraries(fractal_box_bench PRIVATE
	fractal_box::engine
	fractal_box::default_warnings
	fractal_box::wrapped_catch2_with_main
	nanobench::nanobench
)
target_include_directories(fractal_box_bench PRIVATE src)

add_test(NAME bench_collision COMMAND $<TARGET_FILE:fractal_box_bench> [b][collision])
add_test(NAME bench_hash COMMAND $<TARGET_FILE:fractal_box_bench> [b][hashing])
add_test(NAME bench_world COMMAND $<TARGET_FILE:fractal_box_bench> [b][world])

add_subdirectory(src)
