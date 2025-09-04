cmake_minimum_required(VERSION 3.16)
project(Webserv42 CXX)

# Use C++98 like your g++ line
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always generate debug symbols in Debug builds
# (CLion will choose Debug by default for debugging)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra -pedantic")

# Headers live here
include_directories(${CMAKE_SOURCE_DIR}/src/config)

# ---- parser lib (optional but tidy) ----
add_library(config_parser
        src/config/config.cpp
)
# If you don’t want a lib, you can link the .cpp directly to the exe (see below).

# ---- test executable ----
add_executable(confParserTest
        test/confParsertest.cpp
)
target_link_libraries(confParserTest PRIVATE config_parser)

# If you’d rather not create a library, replace the two blocks above with:
# add_executable(confParserTest test/confParsertest.cpp src/config/config.cpp)
# target_include_directories(confParserTest PRIVATE ${CMAKE_SOURCE_DIR}/src/config)
