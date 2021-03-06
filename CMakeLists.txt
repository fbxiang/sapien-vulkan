cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vulkanrenderer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-o0 -g3 -Wall -Wno-deprecated-declarations")
set(CMAKE_CXX_FLAGS_RELEASE "-o3 -g0 -Wall -Wno-deprecated-declarations")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("-- DEBUG Build")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
    add_definitions(-DVK_VALIDATION)
    add_definitions(-D_DEBUG)
else ()
    message("-- RELEASE Build")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
    add_definitions(-DNDEBUG)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(assimp REQUIRED)
find_package(Vulkan REQUIRED)
file(GLOB_RECURSE RENDER_SRC "src/*.cpp")

set(ON_SCREEN TRUE CACHE BOOL "Vulkan renderer with on screen rendering") 
set(COMPILE_SPV_SHADER FALSE CACHE BOOL "True to recompile the SPV shaders")
include_directories("include")
include_directories("$ENV{VULKAN_SDK}/include")

if (${ON_SCREEN}) 
    find_package(glfw3 REQUIRED)
    add_definitions(-DON_SCREEN)
    include_directories("3rd_party/imgui" "3rd_party/imgui/examples")
    file(GLOB GUI_SRC "3rd_party/imgui/*.cpp"
        "3rd_party/imgui/examples/imgui_impl_glfw.cpp"
        "3rd_party/imgui/examples/imgui_impl_vulkan.cpp" )
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/spv)

if (COMPILE_SPV_SHADER)
add_custom_target(glsl
    COMMAND glslc -c ${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.vert ${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.frag
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/spv
    )
else()
    add_custom_target(glsl COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/spv/* ${CMAKE_BINARY_DIR}/spv)
endif()

add_library(sapien-vulkan STATIC ${RENDER_SRC} ${GUI_SRC})
target_link_libraries(sapien-vulkan ${ASSIMP_LIBRARIES} glfw3 dl pthread Vulkan::Vulkan spdlog)
add_dependencies(sapien-vulkan glsl)

add_executable(main app/main.cpp)
target_link_libraries(main sapien-vulkan stdc++fs)
