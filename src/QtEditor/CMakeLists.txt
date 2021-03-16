find_package(Qt5 COMPONENTS Gui REQUIRED)

file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")
add_executable(SazavaEditor ${SOURCES})
target_link_libraries(SazavaEditor PRIVATE Qt5::Gui SazavaLib)
