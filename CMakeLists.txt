cmake_minimum_required(VERSION 3.10)
project(RivenCompiler VERSION 0.9.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(riven
    cli/main.cpp
    lexer/lexer.cpp
    parser/parser.cpp
    semantic/semantic.cpp
)

target_include_directories(riven PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/semantic
    ${CMAKE_CURRENT_SOURCE_DIR}/ast
    ${CMAKE_CURRENT_SOURCE_DIR}/hir
    ${CMAKE_CURRENT_SOURCE_DIR}/mir
    ${CMAKE_CURRENT_SOURCE_DIR}/lir
    ${CMAKE_CURRENT_SOURCE_DIR}/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/formatter
    ${CMAKE_CURRENT_SOURCE_DIR}/linter
    ${CMAKE_CURRENT_SOURCE_DIR}/package_manager
    ${CMAKE_CURRENT_SOURCE_DIR}/doc_generator
    ${CMAKE_CURRENT_SOURCE_DIR}/language_server
    ${CMAKE_CURRENT_SOURCE_DIR}/debugger
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework
)

# Installation details
install(TARGETS riven DESTINATION bin)
