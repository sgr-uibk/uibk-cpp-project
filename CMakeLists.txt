cmake_minimum_required(VERSION 3.20)
project(TankGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
if(NOT WIN32)
    set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE ALWAYS)
endif()
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    ON
    FIND_PACKAGE_ARGS NAMES SFML COMPONENTS Graphics Network Audio
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.16.0
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(spdlog)

include_directories(${CMAKE_SOURCE_DIR}/src)
set(TEST_ROOT ${CMAKE_SOURCE_DIR}/test)

file(GLOB STATE_SRCS src/*/*State.cpp)
file(GLOB CLIENT_SRCS src/*/*Client.cpp)
file(GLOB SERVER_SRCS src/*/*Server.cpp)

add_executable(TankGame
    src/main.cpp
    src/Menu.cpp
    ${STATE_SRCS}
    ${CLIENT_SRCS}
    ${SERVER_SRCS}
    src/Utilities.cpp
    src/HealthBar.cpp
)

add_executable(Client ${TEST_ROOT}/Client.cpp ${STATE_SRCS} ${CLIENT_SRCS} src/Utilities.cpp)
add_executable(Server ${TEST_ROOT}/Server.cpp ${STATE_SRCS} ${SERVER_SRCS} src/Utilities.cpp)

add_library(common_deps INTERFACE)
target_link_libraries(common_deps
    INTERFACE
    SFML::Graphics
    SFML::Network
    SFML::Audio
    spdlog::spdlog
)
target_link_libraries(TankGame PRIVATE common_deps)
target_link_libraries(Client PRIVATE common_deps)
target_link_libraries(Server PRIVATE common_deps)
