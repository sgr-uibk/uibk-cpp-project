cmake_minimum_required(VERSION 3.20)
project(TankGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# These two are needed only for clang-uml (class) diagram generation
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    string(REGEX REPLACE "/W[1-3]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
    add_compile_options(-Wall -Wextra)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.16.0
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(json)

include_directories(${CMAKE_SOURCE_DIR}/src)

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
    src/Map/MapData.h
    src/Map/MapParser.h
    src/Map/MapParser.cpp
)

add_executable(Server src/Server.cpp ${STATE_SRCS} ${SERVER_SRCS} src/Utilities.cpp src/Map/MapParser.cpp)

add_library(common_deps INTERFACE)
target_link_libraries(common_deps
    INTERFACE
    SFML::Graphics
    SFML::Network
    SFML::Audio
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
target_link_libraries(TankGame PRIVATE common_deps)
target_link_libraries(Server PRIVATE common_deps)
