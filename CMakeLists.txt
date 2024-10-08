cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if (WIN32)
    set(USERHOME $ENV{USERPROFILE})
else ()
    set(USERHOME $ENV{HOME})
endif ()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(elijah-payload C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

file(GLOB_RECURSE elijah_SRC CONFIGURE_DEPENDS "src/*.cpp")
add_executable(elijah-payload ${elijah_SRC}
        src/main.h
        src/status_manager.h
        src/status_manager.cpp
        src/usb_communication.h
        src/usb_communication.cpp
        src/sensors/bmp_280/bmp_280.cpp
        src/sensors/bmp_280/bmp_280.h
        src/sensors/hmc_5883l/hmc_5883l.cpp
        src/sensors/hmc_5883l/hmc_5883l.h
        src/byte_util.cpp
        src/byte_util.h
        src/sensors/mpu_6050/mpu_6050.cpp
        src/sensors/mpu_6050/mpu_6050.h)

file(GLOB elijah_PIO CONFIGURE_DEPENDS "src/pio/*.pio")
pico_generate_pio_header(${PROJECT_NAME} ${elijah_PIO})

pico_set_program_name(elijah-payload "elijah-payload")
pico_set_program_version(elijah-payload "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(elijah-payload 0)
pico_enable_stdio_usb(elijah-payload 1)

# Add the standard library to the build
target_link_libraries(elijah-payload
        pico_stdlib)

target_include_directories(elijah-payload PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(elijah-payload
        hardware_i2c
        hardware_pio
        pico_multicore
)

pico_add_extra_outputs(elijah-payload)

