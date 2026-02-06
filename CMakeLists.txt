cmake_minimum_required(VERSION 4.1)
project(expresc VERSION 0.1.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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


add_library(expresc STATIC
        src/expresc.c
        src/parsing/lexer.c
        src/parsing/lexer.h
        src/parsing/parser.c
        src/parsing/parser.h)

target_include_directories(expresc PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_executable(expresc-cli
        src/repl/main.c
)

target_link_libraries(expresc-cli PRIVATE expresc)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if (ipo_supported)
    set_target_properties(expresc PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_target_properties(expresc-cli PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif ()

if (EXPRESC_ENABLE_MARCH_NATIVE)
    target_compile_options(expresc
            PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-march=native>
    )
endif ()

# Apply flags by configuration — library
target_compile_options(expresc PRIVATE
        $<$<CONFIG:Release>:${RELEASE_CFLAGS}>
        $<$<CONFIG:Debug>:${DEBUG_CFLAGS}>
)

# Apply flags by configuration — CLI executable
target_compile_options(expresc-cli PRIVATE
        $<$<CONFIG:Release>:${RELEASE_CFLAGS}>
        $<$<CONFIG:Debug>:${DEBUG_CFLAGS}>
)

# Apply macros by configuration
target_compile_definitions(expresc PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:DEBUG>
)

# LTO link flags on the executable (not the static library)
target_link_options(expresc-cli PRIVATE
        $<$<CONFIG:Release>:-flto>
)