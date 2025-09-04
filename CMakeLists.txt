cmake_minimum_required(VERSION 3.16)
project(Webserv42 CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# headers are in src/config
include_directories(${CMAKE_SOURCE_DIR}/src/config)

# ---- test executable ----
add_executable(confParserTest
        test/confParsertest.cpp
        src/config/config.cpp
)
# (If you prefer a lib, you can split it later.)
