cmake_minimum_required(VERSION 4.1)
project(expresc VERSION 0.1.0 LANGUAGES C)

set(RELEASE_CFLAGS
        -O3 -funroll-loops -fno-strict-aliasing -fno-unsafe-math-optimizations
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(DEBUG_CFLAGS
        -Og -g
        -Wall -Wextra -Wpedantic
        -Wshadow -Wcast-align -Wconversion -Wsign-conversion -Wfloat-equal
        -Wpointer-arith -Wundef -Wlogical-op -Wformat=2
)

option(EXPRESC_ENABLE_MARCH_NATIVE
        "Enable -march=native for this library"
        OFF)


add_library(expresc STATIC src/expresc.c
        src/repl/main.c)

add_executable(expresc-cli
        src/repl/main.c
)

include_directories(${PROJECT_SOURCE_DIR}/include)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if (ipo_supported)
    set_target_properties(expresc PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif ()

if (EXPRESC_ENABLE_MARCH_NATIVE)
    target_compile_options(expresc
            PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-march=native>
    )
endif ()

# Apply flags by configuration
target_compile_options(expresc PRIVATE
        $<$<CONFIG:Release>:${RELEASE_CFLAGS}>
        $<$<CONFIG:Debug>:${DEBUG_CFLAGS}>
)

# Apply macros by configuration
target_compile_definitions(expresc PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:DEBUG>
)

# LTO: -flto often needs to be on the link step too
target_link_options(expresc PRIVATE
        $<$<CONFIG:Release>:-flto>
)