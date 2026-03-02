target_sources(fractal_box_tests
	PRIVATE
		FILE_SET private_headers
		TYPE HEADERS
		FILES
			hashing_fmt.hpp
			test_helpers.hpp
			type_index_fmt.hpp
)
