include_guard(GLOBAL)

# TODO: Split into two functions: enable_options and enable_compiler_flags
function(enable_sanitizer_options option_prefix option_category namespace)
	add_library(${namespace}_sanitizer_options INTERFACE)
	add_library(${namespace}::sanitizer_options ALIAS ${namespace}_sanitizer_options)

	# TODO: MSVC
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
		OR (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
	)
		option(${option_prefix}_ENABLE_COVERAGE
			"${option_prefix}: Enable coverage reporting for gcc/clang" OFF)

		if(${option_prefix}_ENABLE_COVERAGE)
			target_compile_options(${namespace}_sanitizer_options INTERFACE --coverage -O0 -g)
			target_link_libraries(${namespace}_sanitizer_options INTERFACE --coverage)
		endif()

		set(sanitizers "")

		option(${option_prefix}_ENABLE_SANITIZER_ADDRESS
			"${option_category}: Enable address sanitizer" OFF)
		if(${option_prefix}_ENABLE_SANITIZER_ADDRESS)
			list(APPEND sanitizers "address")
		endif()

		option(${option_prefix}_ENABLE_SANITIZER_LEAK
			"${option_category}: Enable leak sanitizer" OFF)
		if(${option_prefix}_ENABLE_SANITIZER_LEAK)
			list(APPEND sanitizers "leak")
		endif()

		option(${option_prefix}_ENABLE_SANITIZER_UB
			"${option_category}: Enable undefined behavior sanitizer" OFF)
		if(${option_prefix}_ENABLE_SANITIZER_UB)
			list(APPEND sanitizers "undefined")
		endif()

		option(${option_prefix}_ENABLE_SANITIZER_THREAD
			"${option_category}: Enable thread sanitizer" OFF)
		if(${option_prefix}_ENABLE_SANITIZER_THREAD)
			if("address" IN_LIST sanitizers OR "leak" IN_LIST sanitizers)
				message(WARNING "${option_category}: Thread sanitizer does not work "
					"with address and leak sanitizer enabled")
			else()
				list(APPEND sanitizers "thread")
			endif()
		endif()

		option(${option_prefix}_ENABLE_SANITIZER_MEMORY
			"${option_category}: Enable memory sanitizer" OFF)
		if(${option_prefix}_ENABLE_SANITIZER_MEMORY AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
			if("address" IN_LIST sanitizers
				OR "thread" IN_LIST sanitizers
				OR "leak" IN_LIST sanitizers
			)
				message(WARNING "${option_category}: Memory sanitizer does not work "
					"with address, thread and leak sanitizer enabled")
			else()
				list(APPEND sanitizers "memory")
			endif()
		endif()

		list(JOIN sanitizers "," list_of_sanitizers)
	endif()

	if(list_of_sanitizers AND NOT "${list_of_sanitizers}" STREQUAL "")
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
			(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
		)
			target_compile_options(${namespace}_sanitizer_options INTERFACE
				-fsanitize=${list_of_sanitizers} -fno-omit-frame-pointer -g)
			target_link_options(${namespace}_sanitizer_options INTERFACE
				-fsanitize=${list_of_sanitizers})
			message(STATUS "${option_category}: Enabled sanitizers: ${list_of_sanitizers}")
		endif()
	endif()
endfunction()

foreach(sanitizer_name address;leak;undefined;memory;thread)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
		OR (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
	)
		add_library(sanitizer_${sanitizer_name} INTERFACE)
		add_library(sanitizer::${sanitizer_name} ALIAS sanitizer_${sanitizer_name})
		target_compile_options(sanitizer_${sanitizer_name} INTERFACE -fsanitize=${sanitizer_name})
		target_link_options(sanitizer_${sanitizer_name} INTERFACE -fsanitize=${sanitizer_name})
	endif()
endforeach()
