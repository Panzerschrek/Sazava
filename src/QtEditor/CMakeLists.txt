find_package(Qt5 COMPONENTS Widgets REQUIRED)

file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")
add_executable(SazavaEditor ${SOURCES})
target_link_libraries(SazavaEditor PRIVATE Qt5::Widgets SDL2ViewerLib)
