target_sources(fractal_box_bench PRIVATE
	collision_benchmarks.cpp
	hashing_benchmarks.cpp
	input_benchmarks.cpp
	world_benchmarks.cpp
	main.cpp
)

add_subdirectory(bench_common)
