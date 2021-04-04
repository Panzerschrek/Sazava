# Search dependencies.
find_package(Vulkan REQUIRED)
find_program(GLSLANGVALIDATOR glslangValidator)
if(NOT GLSLANGVALIDATOR)
	message(FATAL_ERROR "glslangValidator not found")
endif()

# Compile shaders.
file(GLOB SHADERS "shaders/*")
foreach(SHADER_FILE ${SHADERS})
	file(RELATIVE_PATH OUT_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER_FILE})
	set(OUT_FILE_BASE ${CMAKE_CURRENT_BINARY_DIR}/${OUT_FILE})
	string(REPLACE ".glsl" ".h" OUT_FILE_H ${OUT_FILE_BASE})
	get_filename_component(VARIABLE_NAME ${SHADER_FILE} NAME)
	string(REPLACE ".glsl" "" VARIABLE_NAME ${VARIABLE_NAME})
	string(REPLACE "." "_" VARIABLE_NAME ${VARIABLE_NAME})
	add_custom_command(
		OUTPUT ${OUT_FILE_H}
		DEPENDS ${SHADER_FILE}
		COMMAND ${GLSLANGVALIDATOR} -V ${SHADER_FILE} --vn ${VARIABLE_NAME} -o ${OUT_FILE_H}
		)

	list(APPEND SHADERS_COMPILED ${OUT_FILE_H})
endforeach()

# Write shader list files.

# Add main executable
file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")
add_library(SazavaLib ${SOURCES} ${SHADERS} ${SHADERS_COMPILED})
target_include_directories(
	SazavaLib
		PUBLIC
			${Vulkan_INCLUDE_DIRS}
			${CMAKE_CURRENT_BINARY_DIR}
		)
target_link_libraries(SazavaLib PUBLIC ${Vulkan_LIBRARIES})
