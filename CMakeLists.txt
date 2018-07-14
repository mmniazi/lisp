cmake_minimum_required(VERSION 3.10)
project(lisp C)

set(CMAKE_C_STANDARD 99)

add_executable(lisp parsing.c libs/mpc.c lval.c eval.c)
target_link_libraries(lisp edit)
