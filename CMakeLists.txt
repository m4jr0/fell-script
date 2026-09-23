cmake_minimum_required(VERSION 3.20)

project(Fell
    VERSION 0.1.0
    LANGUAGES CXX
)

add_executable(fell
    tools/fell/main.cc
)

target_compile_features(fell PRIVATE cxx_std_20)

set_target_properties(fell PROPERTIES
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(fell PRIVATE
        /W4
        /WX
        /permissive-
    )
else()
    target_compile_options(fell PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
endif()