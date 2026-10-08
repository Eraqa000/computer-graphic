cmake_minimum_required(VERSION 3.15)
project(cg LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw glm)

# glad (OpenGL 3.3 core, generated at gen.glad.sh)
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

# stb (header-only)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE external/stb)

add_executable(cg main.cpp)
target_link_libraries(cg PRIVATE glad glfw glm::glm stb)

if(APPLE)
    target_link_libraries(cg PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
endif()
