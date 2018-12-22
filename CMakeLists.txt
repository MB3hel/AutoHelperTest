project(AutoHelperTest)
cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_STANDARD 11)

# This macro converts a MSYS2 path to a windows path (with forward slash). Needed for SDL2 and maybe for other libs installed with MSYS2
macro(MSYS_TO_WIN _path)
	find_program(CYGPATH cygpath)
	if(CYGPATH AND (CYGWIN OR MSYS))
		execute_process(COMMAND ${CYGPATH} -m ${${_path}} OUTPUT_VARIABLE ${_path})
		string (STRIP ${${_path}} ${_path})
	endif()
endmacro()

# Include ucm (Usefull Cmake Macros for setting MSVC runtime)
include(${PROJECT_SOURCE_DIR}/cmake/ucm.cmake)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	set(NO_CONAN OFF CACHE INTERNAL "")
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup(TARGETS)

	# Use the same runtime (MSVC MD[d] or MT[d]) as conan was configured to use
	if(MSVC AND ("${CONAN_SETTINGS_COMPILER_RUNTIME}" MATCHES "MT" OR "${CONAN_SETTINGS_COMPILER_RUNTIME}" MATCHES "MTd"))
		ucm_set_runtime(STATIC) # This will use /MT and /MTd with MSVC
	elseif(MSVC)
		ucm_set_runtime(DYNAMIC) # This will use /MD and /MDd with MSVC
	endif()
else()
	set(NO_CONAN ON CACHE INTERNAL "")
	#message(WARNING "Not using conan for dependancy management. This is **NOT** recommended!!!")
endif()

# Use the same runtime (MSVC MD[d] or MT[d]) as conan was configured to use
if(MSVC AND ("${CONAN_SETTINGS_COMPILER_RUNTIME}" MATCHES "MT" OR "${CONAN_SETTINGS_COMPILER_RUNTIME}" MATCHES "MTd"))
    ucm_set_runtime(STATIC) # This will use /MT and /MTd with MSVC
elseif(MSVC)
    ucm_set_runtime(DYNAMIC) # This will use /MD and /MDd with MSVC
endif()

# Build the resource generator
add_subdirectory(resources)

# Build the main program
add_subdirectory(AutoTest)
