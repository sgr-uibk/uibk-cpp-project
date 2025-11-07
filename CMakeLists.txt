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
    FIND_PACKAGE_ARGS NAMES spdlog
)
FetchContent_MakeAvailable(spdlog)

include_directories(${CMAKE_SOURCE_DIR}/src)
set(TEST_ROOT ${CMAKE_SOURCE_DIR}/test)

file(GLOB STATE_SRCS src/*/*State.cpp)
file(GLOB CLIENT_SRCS src/*/*Client.cpp)

add_executable(GameServer ${TEST_ROOT}/GameServer.cpp ${STATE_SRCS} src/Utilities.cpp src/Map/MapLoader.cpp src/Map/Tile.cpp)
add_executable(GameClient ${TEST_ROOT}/GameClient.cpp ${STATE_SRCS} ${CLIENT_SRCS} src/Utilities.cpp src/Map/MapLoader.cpp src/Map/Tile.cpp)
add_executable(LobbyServer ${TEST_ROOT}/LobbyServer.cpp src/Utilities.cpp)
add_executable(LobbyClient ${TEST_ROOT}/LobbyClient.cpp src/Utilities.cpp)

add_library(common_deps INTERFACE)
target_link_libraries(common_deps
    INTERFACE
    SFML::Graphics
    SFML::Network
    spdlog::spdlog
)
target_link_libraries(LobbyServer PRIVATE common_deps)
target_link_libraries(LobbyClient PRIVATE common_deps)
target_link_libraries(GameServer PRIVATE common_deps)
target_link_libraries(GameClient PRIVATE common_deps)
