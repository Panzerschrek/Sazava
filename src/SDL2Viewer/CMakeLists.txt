if(WIN32)
	set(SDL2_INCLUDE_DIRS ${SDL2_DIR}/include)
	set(SDL2_LIBRARIES ${SDL2_DIR}/lib/x64/SDL2.lib)
else()
	find_package(SDL2 REQUIRED)
endif()
find_package(Vulkan REQUIRED)
find_program(GLSLANGVALIDATOR glslangValidator)
if(NOT GLSLANGVALIDATOR)
	message(FATAL_ERROR "glslangValidator not found")
endif()

# Add main executable
file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")
add_executable(Sazava ${SOURCES})
target_include_directories(
	Sazava
		PRIVATE
			${SDL2_INCLUDE_DIRS}
		)
target_link_libraries(
	Sazava
		PRIVATE
			SazavaLib
			${SDL2_LIBRARIES}
	)
