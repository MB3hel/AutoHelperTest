# Include the icon and resource directories (for xpm icons and resources.h)
include_directories(${PROJECT_SOURCE_DIR}/icons ${PROJECT_SOURCE_DIR}/resources ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "include/*.h"
    "include/*.hpp"
)

add_executable(AutoTest
               ${WIN_RESOURCE_FILE}
               ${SOURCES})

# Extra libraries (not handled by conan/find_package)
set(EXTRA_LIBS )
if(MINGW)
    #set(EXTRA_LIBS ${EXTRA_LIBS} extra_lib)
endif()
if(UNIX)
    #set(EXTRA_LIBS ${EXTRA_LIBS} extra_lib)
endif()

if(${NO_CONAN})
    # Find required packages/libraries manually
	# find_package(package REQUIRED)
    # include_directories(${package_INCLUDE_DIRS})
    # target_link_libraries(AutoTest ${package_LIBRARIES}  ${EXTRA_LIBS})
else()
	# Link with conan libraries
    #target_link_libraries(AutoTest CONAN_PKG::lib_name ${EXTRA_LIBS})
endif()

#CPack
if(WIN32)
    install(TARGETS AutoTest
            RUNTIME
            DESTINATION programs
            COMPONENT applications)
elseif(APPLE)
    install(TARGETS AutoTest
            BUNDLE DESTINATION .)
else()
    install(TARGETS AutoTest
            RUNTIME DESTINATION bin)
endif()
