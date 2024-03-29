# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/components
    )

#### Define Macros
## OTA_UPGRADE | Humidity | Temperature | BATTERY | LIGHT_SLEEP | DEEP_SLEEP | SENSOR_WATERLEAK
add_definitions(-DSENSOR_MAP=0b1111100)
add_definitions(-DDEVICE_ENDPOINT=10)

## Channel and TX-Power
add_definitions(-DTX_POWER=0)

## KEEP ALIVE
add_definitions(-DED_KEEP_ALIVE=4000)

## OTA MACROS
add_definitions(-DESP_OTA_CLIENT_ENDPOINT=5)
# Define HEX 0x0101 or DEC 257
add_definitions(-DOTA_UPGRADE_HW_VERSION=0x0101)
add_definitions(-DOTA_UPGRADE_DOWNLOADED_FILE_VERSION=0x01010107)
add_definitions(-DOTA_UPGRADE_RUNNING_FILE_VERSION=0x01010107)
add_definitions(-DOTA_UPGRADE_IMAGE_TYPE=0x1011)
add_definitions(-DOTA_UPGRADE_MANUFACTURER=0x1001)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sensor_device)
