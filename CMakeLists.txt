cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vulkanrenderer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-o0 -g3 -Wall")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)
find_package(Vulkan REQUIRED)
file(GLOB_RECURSE RENDER_SRC "src/*.cpp")

add_definitions(-DVK_VALIDATION)
include_directories("src")

include_directories("3rdparty/imgui" "3rdparty/imgui/examples")
include_directories("3rdparty/imgui" "3rdparty/imgui/examples")

file(GLOB GUI_SRC "3rdparty/imgui/*.cpp"
  "3rdparty/imgui/examples/imgui_impl_glfw.cpp"
  "3rdparty/imgui/examples/imgui_impl_vulkan.cpp" )

add_custom_target(glsl COMMAND glslc -c ${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.vert ${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.frag)

add_executable(main ${RENDER_SRC} ${GUI_SRC})
target_link_libraries(main ${ASSIMP_LIBRARIES} glfw Vulkan::Vulkan spdlog stdc++fs)
add_dependencies(main glsl)
