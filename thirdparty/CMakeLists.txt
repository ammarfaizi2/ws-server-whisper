set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")
include("$CACHE{CMAKE_TOOLCHAIN_FILE}")

# Third party dependencies using vcpkg
# 1. fmt
# 2. websocketpp
find_package(fmt REQUIRED GLOBAL)
find_package(websocketpp REQUIRED GLOBAL)

# Third party dependencies
# 1. whispercpp


# Websocketpp
# add_library(websocketpp-HeaderOnly INTERFACE)
# add_dependencies(websocketpp-HeaderOnly Boost_build)
# target_include_directories(websocketpp-HeaderOnly INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/websocketpp)

#Whispercpp
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(whisper.cpp)