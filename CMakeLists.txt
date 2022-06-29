cmake_minimum_required(VERSION 3.5)
project(tex-preprocessor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_ARGS "${CMAKE_ARGS} arg1:string=\"test.tex\"")
set(CMAKE_ARGS "${CMAKE_ARGS} arg1:string=\"_test.tex\"")

include_directories(.)

add_executable(
    tex-preprocessor
    main.cpp
    Error.cpp
    Defines.cpp
    Coordinate.cpp
    FileHandler.cpp
    Lexer.cpp
    Node.cpp
    Value.cpp
    basic_HM.cpp
)
