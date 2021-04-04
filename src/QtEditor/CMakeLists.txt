find_package(Qt5 COMPONENTS Widgets REQUIRED)

option(SAZAVA_EDITOR_USE_QT_VULKAN "Use Qt Vulkan for 3d view output" ON)

file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")
file(GLOB_RECURSE SOURCES_QT_VULKAN "CentralWidgetQtVulkan.cpp")
file(GLOB_RECURSE SOURCES_SDL_VULKAN "CentralWidgetSDLVulkan.cpp")

if(SAZAVA_EDITOR_USE_QT_VULKAN)
	list(REMOVE_ITEM SOURCES ${SOURCES_SDL_VULKAN})
else()
	list(REMOVE_ITEM SOURCES ${SOURCES_QT_VULKAN})
endif()

add_executable(SazavaEditor ${SOURCES})

if(SAZAVA_EDITOR_USE_QT_VULKAN)
	target_link_libraries(SazavaEditor PRIVATE Qt5::Widgets SazavaLib)
else()
	target_link_libraries(SazavaEditor PRIVATE Qt5::Widgets SDL2ViewerLib)
endif()
