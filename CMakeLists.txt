cmake_minimum_required(VERSION 3.20)
project(SpeechRecognition VERSION 1.0.0)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include_directories(include)

add_subdirectory(thirdparty)

add_executable(ws-server ws-server.cpp)
target_link_libraries(ws-server PRIVATE 
    whisper
    pthread
    websocketpp::websocketpp
    fmt::fmt-header-only
)