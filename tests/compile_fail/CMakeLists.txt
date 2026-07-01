function(microtimer_add_compile_fail_test name source regex)
    set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    try_compile(
        ${name}_RESULT
        "${binary_dir}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
        CMAKE_FLAGS
            "-DCMAKE_C_STANDARD=99"
            "-DCMAKE_C_STANDARD_REQUIRED=ON"
            "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"
        OUTPUT_VARIABLE ${name}_OUTPUT)

    if(${name}_RESULT)
        message(FATAL_ERROR "compile-fail test '${name}' unexpectedly compiled")
    endif()

    string(REGEX MATCH "${regex}" ${name}_MATCH "${${name}_OUTPUT}")
    if("${${name}_MATCH}" STREQUAL "")
        message(FATAL_ERROR "compile-fail test '${name}' did not emit expected diagnostic")
    endif()
endfunction()

microtimer_add_compile_fail_test(max_timers_zero max_timers_zero.c "MTIMER_MAX_TIMERS must be >= 1")
microtimer_add_compile_fail_test(max_timers_negative max_timers_negative.c "MTIMER_MAX_TIMERS must be >= 1")
microtimer_add_compile_fail_test(max_timers_256 max_timers_256.c "MTIMER_MAX_TIMERS must be <= 255")
microtimer_add_compile_fail_test(max_timers_too_large max_timers_too_large.c "MTIMER_MAX_TIMERS must be <= 255")
