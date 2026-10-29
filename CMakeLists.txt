cmake_minimum_required(VERSION 3.28)
project(osi_lab3 C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "This project is for Windows only")
endif()

add_compile_options(-Wall -Wextra)

add_executable(lab3
        src/main.c
        src/matrix.c
        src/det.c
        src/common.c
)

if(MINGW)
    target_link_libraries(lab3 PRIVATE kernel32)
endif()

set_target_properties(lab3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
