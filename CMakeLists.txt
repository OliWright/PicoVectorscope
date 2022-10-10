cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_vectorscope C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

#set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

#include(example_auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )


add_executable(vectorscope)

pico_generate_pio_header(vectorscope ${CMAKE_CURRENT_LIST_DIR}/vector.pio)
pico_generate_pio_header(vectorscope ${CMAKE_CURRENT_LIST_DIR}/points.pio)
pico_generate_pio_header(vectorscope ${CMAKE_CURRENT_LIST_DIR}/idle.pio)

target_sources(vectorscope PRIVATE
        buttons.cpp
        cooldemo0.cpp
        cooldemo1.cpp
        cooldemo2.cpp
        cooldemo3.cpp
        cooldemo4.cpp
        dacout.cpp
        dacoutputsm.cpp
        fixedpoint.cpp
        displaylist.cpp
        gameshapes.cpp
        ledstatus.cpp
        log.cpp
        main.cpp
        shapes.cpp
        text.cpp
        transform2d.cpp
)

target_link_libraries(vectorscope PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_dma
        hardware_pwm
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(vectorscope 1)
pico_enable_stdio_uart(vectorscope 0)

pico_add_extra_outputs(vectorscope)

# add url via pico_set_program_url
#example_auto_set_url(hello_pio)
