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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(blink_any C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add the pico_fft library subdirectory
add_subdirectory(pico_fft)

# Add executable. Default name is the project name, version 0.1
if (NOT PICO_CYW43_SUPPORTED)
    message("Only building blink_any for non W boards as PICO_CYW43_SUPPORTED is not set")
endif()

add_executable(blink_any
    blink_any.c
)

# Add include directory for FFT headers - using absolute paths to be sure
target_include_directories(blink_any PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/pico_fft/src/include/pico"
)

# Link libraries including pico_fft
target_link_libraries(
    blink_any 
    pico_stdlib 
    hardware_adc 
    hardware_timer
    hardware_spi
    pico_fft
)

# enable usb output, disable uart output
pico_enable_stdio_usb(blink_any 1)
pico_enable_stdio_uart(blink_any 0)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(blink_any pico_cyw43_arch_none)
endif()


