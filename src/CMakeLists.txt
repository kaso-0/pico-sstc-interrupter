cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(usb_midi_project)
pico_sdk_init()

add_executable(midi_main
    midi_main.c
)

target_include_directories(midi_main PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(midi_main
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(midi_main)
