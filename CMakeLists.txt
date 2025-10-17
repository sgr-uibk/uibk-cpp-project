cmake_minimum_required(VERSION 3.20)
project(TankGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent für SDL2 & SDL2_image
include(FetchContent)

# SDL2
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.3
)
FetchContent_MakeAvailable(SDL2)

# SDL2_image
FetchContent_Declare(
    SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.6.2
)
FetchContent_MakeAvailable(SDL2_image)

# Quellen
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(TankGame ${SOURCES})

target_link_libraries(TankGame PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2_image)

# OpenGL für SDL2 (optional)
if(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(TankGame PRIVATE OpenGL::GL)
elseif(WIN32)
    target_link_libraries(TankGame PRIVATE OpenGL32)
endif()
