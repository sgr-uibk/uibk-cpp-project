cmake_minimum_required(VERSION 3.20)
project(TankGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE ALWAYS)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    ON
    FIND_PACKAGE_ARGS NAMES SFML COMPONENTS Graphics Window System
)
FetchContent_MakeAvailable(SFML)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(TankGame ${SOURCES})

target_compile_features(TankGame PRIVATE cxx_std_20)
target_link_libraries(TankGame PRIVATE SFML::Graphics)
