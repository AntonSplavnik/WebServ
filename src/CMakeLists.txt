cmake_minimum_required(VERSION 3.20)
project(Webserv42 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_executable(confParserTest
        src/test/confParsertest.cpp
        src/config/config.cpp
)
target_include_directories(confParserTest PRIVATE src/config)
