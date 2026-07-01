cmake_minimum_required(VERSION 3.21)
project(microtimer_find_package_consumer LANGUAGES C)

find_package(microtimer CONFIG REQUIRED)

add_executable(find_package_consumer main.c)
target_link_libraries(find_package_consumer PRIVATE microtimer::microtimer)
