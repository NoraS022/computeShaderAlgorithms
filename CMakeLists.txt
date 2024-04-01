cmake_minimum_required(VERSION "3.20")
project(compute)
set(CMAKE_CXX_STANDARD 23) # CPP version
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-W -Wall -g")

# Project itself
file(GLOB PROJECT_SRCS src/*.h src/*.cpp)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/glfw" ${CMAKE_CURRENT_BINARY_DIR}/gltf)
include_directories(SYSTEM "./external/glfw/include")

# gl3w
#aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/src" PROJECT_SRCS)
#include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/include")
aux_source_directory("./external/gl3w/src" PROJECT_SRCS)
include_directories(SYSTEM "./external/gl3w/include")

add_executable(${PROJECT_NAME} ${PROJECT_SRCS}) # Do once

# OpenGL
set(PROJECT_LIBRARIES)
find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
  include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
  set(PROJECT_LIBRARIES ${PROJECT_LIBRARIES} ${OPENGL_LIBRARIES})
endif(OPENGL_FOUND)

target_link_libraries(${PROJECT_NAME} glfw ${PROJECT_LIBRARIES} ${GLFW_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/")


