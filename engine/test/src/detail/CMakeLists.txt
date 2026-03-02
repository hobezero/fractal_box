target_sources(fractal_box_tests
	PRIVATE
		FILE_SET private_headers
		TYPE HEADERS
		FILES
			type_index_tu_helpers.hpp
	PRIVATE
		type_index_tu_helpers.cpp
)
