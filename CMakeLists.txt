cmake_minimum_required(VERSION 3.22)

project(Mittens VERSION 1.0 DESCRIPTION "Mittens: Discord bot that outputs your match result!")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)
FetchContent_Declare(sqlite3 URL https://www.sqlite.org/2025/sqlite-amalgamation-3500000.zip)
FetchContent_MakeAvailable(sqlite3)
add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/)


find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL Include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")

#dpp
find_package(DPP REQUIRED CONFIG)

#nlohmann-json
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(${PROJECT_NAME}
        src/Player.cpp
        src/RiotAPI.cpp
        src/bot.cpp
        src/data.cpp
        src/helpers.cpp
        src/main.cpp
        src/Worker.cpp
        src/MatchInfo.cpp
        src/Persistence.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
        ${nlohmann_json_INCLUDE_DIRS}
)

#Linking
target_link_libraries(${PROJECT_NAME} PRIVATE
        dpp::dpp
	OpenSSL::SSL
	OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        sqlite3
)

add_executable(data_writer EXCLUDE_FROM_ALL
        src/data_writer.cpp
        src/data.cpp
        src/Player.cpp
        src/helpers.cpp
)

set_target_properties(data_writer PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

target_include_directories(data_writer PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
        ${nlohmann_json_INCLUDE_DIRS}
)

#Linking
target_link_libraries(data_writer PRIVATE
        nlohmann_json::nlohmann_json
        dpp::dpp
	OpenSSL::SSL
	OpenSSL::Crypto
)