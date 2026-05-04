cmake_minimum_required(VERSION 3.16)
project(OS2_AS2 C)

set(CMAKE_C_STANDARD 17)

add_executable(OS2_AS2
        main.c
        helper_functions.c
        queue.c
        supermarket.c
)

target_compile_options(OS2_AS2 PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wshadow
)

find_package(Threads REQUIRED)

target_link_libraries(OS2_AS2 PRIVATE Threads::Threads)
