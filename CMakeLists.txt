cmake_minimum_required(VERSION 3.21)

project(microtimer VERSION 1.0.0 LANGUAGES C)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(MICROTIMER_MAX_TIMERS "8" CACHE STRING "Resolved timer capacity for this build")
option(MICROTIMER_BUILD_TESTS "Build microtimer tests" ${PROJECT_IS_TOP_LEVEL})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mtimer_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/mtimer_config.h"
    @ONLY)

add_library(microtimer src/mtimer.c)
add_library(microtimer::microtimer ALIAS microtimer)

target_compile_features(microtimer PUBLIC c_std_99)
target_include_directories(microtimer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

string(REGEX MATCHALL "-fsanitize=[^ ]+" MICROTIMER_SANITIZER_FLAGS "${CMAKE_C_FLAGS}")
if(MICROTIMER_SANITIZER_FLAGS)
    target_link_options(microtimer INTERFACE ${MICROTIMER_SANITIZER_FLAGS})
endif()

set_target_properties(microtimer PROPERTIES
    EXPORT_NAME microtimer
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS microtimer
    EXPORT microtimerTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mtimer.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/mtimer_config.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT microtimerTargets
    FILE microtimerTargets.cmake
    NAMESPACE microtimer::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microtimer")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/microtimerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/microtimerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/microtimerConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microtimer")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/microtimerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/microtimerConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microtimer")

if(MICROTIMER_BUILD_TESTS)
    enable_language(CXX)
    enable_testing()
    add_subdirectory(tests)
endif()
