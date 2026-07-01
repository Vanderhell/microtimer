cmake_minimum_required(VERSION 3.21)
project(microtimer_add_subdirectory_consumer LANGUAGES C)

add_subdirectory(../.. microtimer-build)

add_executable(add_subdirectory_consumer main.c)
target_link_libraries(add_subdirectory_consumer PRIVATE microtimer::microtimer)
