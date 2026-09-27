add_executable(lu_test test.cpp)

target_link_libraries(lu_test PRIVATE lu)

if(LIBLU_USE_GPERFTOOLS)
    find_library(GPERFTOOLS_PROFILER profiler)
    if(GPERFTOOLS_PROFILER)
        target_compile_definitions(lu_test PRIVATE LIBLU_USE_GPERFTOOLS=1)
        target_link_libraries(lu_test PRIVATE ${GPERFTOOLS_PROFILER})
    else()
        message(WARNING "gperftools profiler not found; building test without it")
    endif()
endif()
