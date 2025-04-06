cmake_minimum_required(VERSION 3.30)
project(cache_FingerPrint C)

set(CMAKE_C_STANDARD 17)

set(SOURCES
        main.c
        utils.c
        memorygrammer.c
        cpu-config.c
)
set(HEADERS
        memorygrammer.h
        cpu-config.h
        utils.h
)

add_executable(cache_FingerPrint ${SOURCES} ${HEADERS})