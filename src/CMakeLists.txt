cmake_minimum_required(VERSION 3.10)

project(Sazava)

# Setup language options.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# Setup compiler options.
if(MSVC)
	add_compile_options(/utf-8 /W3 /WX)
	add_definitions(
		-DNOMINMAX
		-D_CRT_SECURE_NO_WARNINGS)
else()
	add_compile_options(-Werror -Wall -Wextra -Wconversion)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

# Search dependencies.
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")
add_executable(Sazava ${SOURCES})
target_include_directories(
	Sazava
		PRIVATE
			${SDL2_INCLUDE_DIRS}
			${Vulkan_INCLUDE_DIRS}
		)
target_link_libraries(
	Sazava
		PRIVATE
			${SDL2_LIBRARIES}
			${Vulkan_LIBRARIES}
	)
