# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Tell Pico SDK to use the local copy of tinyusb
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-tinyusb-msc-panic C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(pico-tinyusb-msc-panic
    pico-tinyusb-msc-panic.c
    usb_descriptors.c
    usb_msc_cb.c
)

pico_set_program_name(pico-tinyusb-msc-panic "pico-tinyusb-msc-panic")
pico_set_program_version(pico-tinyusb-msc-panic "0.1")

# no_flash means the target is to run from RAM
pico_set_binary_type(pico-tinyusb-msc-panic no_flash)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico-tinyusb-msc-panic 0)
pico_enable_stdio_usb(pico-tinyusb-msc-panic 1)

# Include RP2350 picotool vendor interface
target_compile_definitions(pico-tinyusb-msc-panic
  PRIVATE
    PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=1
    PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL=1
    PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_BY_VENDOR_CMD=1
    PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK=0 # XXX HACK FIXME
)
target_sources(pico-tinyusb-msc-panic PRIVATE
    ${PICO_SDK_PATH}/src/rp2_common/pico_stdio_usb/reset_interface.c
)
# Add the standard library to the build
target_link_libraries(pico-tinyusb-msc-panic
        pico_stdlib
        pico_stdio_usb
)

# Add the standard include files to the build
target_include_directories(pico-tinyusb-msc-panic PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(pico-tinyusb-msc-panic
        pico_cyw43_arch_none
        tinyusb_device
        tinyusb_board
        )

pico_add_extra_outputs(pico-tinyusb-msc-panic)
