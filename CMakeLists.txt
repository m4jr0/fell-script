cmake_minimum_required(VERSION 3.20)

project(Fell
    VERSION 0.1.0
    LANGUAGES CXX
)

function(fell_configure_target target)
    target_compile_features(${target} PUBLIC cxx_std_20)

    set_target_properties(${target} PROPERTIES
        CXX_EXTENSIONS OFF
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /WX
            /permissive-
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
        )
    endif()
endfunction()

add_library(fell_core
    src/core/char.cc
    src/core/logger.cc
)

target_include_directories(fell_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

fell_configure_target(fell_core)

add_library(fell_compiler
    src/compiler/ast.cc
    src/compiler/bytecode_compiler.cc
    src/compiler/compiler.cc
    src/compiler/diagnostic.cc
    src/compiler/ir.cc
    src/compiler/ir_builder.cc
    src/compiler/lexer.cc
    src/compiler/parser.cc
    src/compiler/semantic_analyzer.cc
    src/compiler/type.cc
)

target_include_directories(fell_compiler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fell_compiler PUBLIC
    fell_core
)

fell_configure_target(fell_compiler)

add_library(fell_runtime
    src/runtime/value.cc
    src/runtime/vm.cc
)

target_include_directories(fell_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fell_runtime PUBLIC
    fell_core
)

fell_configure_target(fell_runtime)

add_executable(fell
    tools/fell/main.cc
)

target_link_libraries(fell PRIVATE
    fell_compiler
    fell_runtime
)

fell_configure_target(fell)
