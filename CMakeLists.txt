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

add_executable(TankGame    src/main.cpp src/GameWorld.cpp src/HealthBar.cpp src/Player.cpp src/Map.cpp src/Utilities.cpp)
add_executable(LobbyServer src/LobbyServer.cpp src/Utilities.cpp)
add_executable(LobbyClient src/LobbyClient.cpp src/Utilities.cpp)

target_compile_features(TankGame    PRIVATE cxx_std_20)
target_compile_features(LobbyServer PRIVATE cxx_std_20)
target_compile_features(LobbyClient PRIVATE cxx_std_20)

target_link_libraries(TankGame    PRIVATE SFML::Graphics SFML::Audio spdlog::spdlog)
target_link_libraries(LobbyServer PRIVATE SFML::Graphics SFML::Network spdlog::spdlog)
target_link_libraries(LobbyClient PRIVATE SFML::Graphics SFML::Network spdlog::spdlog)
