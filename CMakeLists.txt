cmake_minimum_required(VERSION 3.20)
project(TankGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_image REQUIRED CONFIG)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(TankGame ${SOURCES})

# Link the libraries
target_link_libraries(TankGame PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2_image::SDL2_image)

# OpenGL for SDL2 (optional)
if(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(TankGame PRIVATE OpenGL::GL)
elseif(WIN32)
    target_link_libraries(TankGame PRIVATE OpenGL32)
endif()

if(WIN32) # link executable against static runtime
    set(MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()
