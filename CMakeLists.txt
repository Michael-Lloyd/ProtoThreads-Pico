cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(Protothreads)

pico_sdk_init()

add_executable(Protothreads ProtoThreads.c)

pico_enable_stdio_usb(Protothreads 0)
pico_enable_stdio_uart(Protothreads 1)

pico_add_extra_outputs(Protothreads)

target_link_libraries(Protothreads 
    pico_stdlib 
    pico_bootsel_via_double_reset 
    pico_multicore)

add_compile_options(-Ofast)
