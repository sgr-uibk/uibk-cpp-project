cmake_minimum_required(VERSION 3.20)
project(TankGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32) # link executable against static runtime
    set(MSVC_RUNTIME_LIBRARY MultiThreaded)
    set(CMAKE_C_RUNTIME_LIBRARY MultiThreaded)
    set(CMAKE_CXX_RUNTIME_LIBRARY MultiThreaded)
endif()

# Output to build/Debug or build/Release
find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(SDL3_mixer QUIET)
if(NOT SDL3_mixer_FOUND) # not in vcpkg
    message(STATUS "SDL3_mixer not found, fetching from GitHub...")

    include(FetchContent)
    FetchContent_Declare(
            SDL3_mixer
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer
            GIT_TAG release-2.8.1
    )
    FetchContent_MakeAvailable(SDL3_mixer)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(TankGame ${SOURCES})

target_link_libraries(TankGame PRIVATE
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    SDL3::SDL3 # main lib as last entry
)
