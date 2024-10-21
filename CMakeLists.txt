# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/components
    )

#### ######### ####
#### FUNKTIONS ####
# Funktion zur Konvertierung einer Binärzahl in eine Dezimalzahl
function(binary_to_decimal binary_number result)
    set(decimal_value 0)
    set(base 1)

    # Binärzahl von rechts nach links verarbeiten
    string(LENGTH "${binary_number}" binary_length)
    math(EXPR last_index "${binary_length} - 1")
    
    foreach(index RANGE ${last_index} 0 -1)
        string(SUBSTRING "${binary_number}" ${index} 1 digit)
        if(digit EQUAL "1")
            math(EXPR decimal_value "${decimal_value} + ${base}")
        endif()
        math(EXPR base "${base} * 2")
    endforeach()

    # Ergebnis zurückgeben
    set(${result} "${decimal_value}" PARENT_SCOPE)
endfunction()

# Funktion zur Formatierung des Ergebnisses auf eine feste Länge
function(format_decimal decimal_number length result)
    string(LENGTH "${decimal_number}" decimal_length)
    if(decimal_length LESS length)
        math(EXPR zeros_to_add "${length} - ${decimal_length}")
        string(REPEAT "0" ${zeros_to_add} leading_zeros)
        set(formatted_result "${leading_zeros}${decimal_number}")
    else()
        set(formatted_result "${decimal_number}")
    endif()

    # Ergebnis zurückgeben
    set(${result} "${formatted_result}" PARENT_SCOPE)
endfunction()
#### FUNKTIONS ####
#### ######### ####

## Set dev or prod
set(DEV_PROD "dev")

#### Define Macros
# add_definitions(-DCONFIG_ZB_ZED=y)
# idf_build_set_property(COMPILE_OPTIONS "-DCDCONFIG_ZB_ZED=y" APPEND)
## First 6 bits are for additional future features
## END_DEVICE(0=ROUTER_DEVICE) | OTA_UPGRADE | Humidity | Temperature | BATTERY | LIGHT_SLEEP | DEEP_SLEEP | SENSOR_WATERLEAK | AUTOMATIC_IRRIGATION | LIGHT_ON_OFF
set(SENSOR_MAP_DEF "1001110001000000")
add_definitions(-DSENSOR_MAP=0b${SENSOR_MAP_DEF})
binary_to_decimal(${SENSOR_MAP_DEF} MODEL_ID_DECIMAL)
format_decimal(${MODEL_ID_DECIMAL} 5 MODEL_ID_FORMATED)
add_definitions(-DMODEL_ID_MAP=${MODEL_ID_FORMATED})
add_definitions(-DDEVICE_ENDPOINT=10)

#### Check SENSOR_MAP_DEF
# Sixth and Seventh bit cannot be at the same time 1.
# Both should be 0 or one of them should be 1.
string(SUBSTRING ${SENSOR_MAP_DEF} 5 1 SIXTH_BIT)
string(SUBSTRING ${SENSOR_MAP_DEF} 6 1 SEVENTH_BIT)

if(SIXTH_BIT EQUAL "1" AND SEVENTH_BIT EQUAL "1")
    message(FATAL_ERROR "Sixth and Seventh bits cannot both be 1 at the same time in SENSOR_MAP_DEF. Please check the SENSOR_MAP_DEF and correct it.")
endif()

#### Set specific sdkconfig
# Check the first number of SENSOR_MAP_DEF
string(SUBSTRING ${SENSOR_MAP_DEF} 0 1 FIRST_NUM)
if(FIRST_NUM EQUAL "1")
    set(SET_SDKCONFIG_DEFAULTS "sdkconfig_defaults/sdkconfig.end_device")
    message(STATUS "Setting SET_SDKCONFIG_DEFAULTS to sdkconfig.end_device")
else()
    set(SET_SDKCONFIG_DEFAULTS "sdkconfig_defaults/sdkconfig.router")
    message(STATUS "Setting SET_SDKCONFIG_DEFAULTS to sdkconfig.router")
endif()

# Check the sixth number of SENSOR_MAP_DEF
string(SUBSTRING ${SENSOR_MAP_DEF} 5 1 SIXTH_NUM)
if(SIXTH_NUM EQUAL "1")
    list(APPEND SET_SDKCONFIG_DEFAULTS "sdkconfig_defaults/sdkconfig.light_sleep")
    message(STATUS "Appending sdkconfig.light_sleep to SET_SDKCONFIG_DEFAULTS")
endif()

# Check the seventh number of SENSOR_MAP_DEF
string(SUBSTRING ${SENSOR_MAP_DEF} 6 1 SEVENTH_NUM)
if(SEVENTH_NUM EQUAL "1")
    list(APPEND SET_SDKCONFIG_DEFAULTS "sdkconfig_defaults/sdkconfig.deep_sleep")
    message(STATUS "Appending sdkconfig.deep_sleep to SET_SDKCONFIG_DEFAULTS")
endif()

# Check the tenth number of SENSOR_MAP_DEF
string(SUBSTRING ${SENSOR_MAP_DEF} 9 1 TENTH_NUM)
if(TENTH_NUM EQUAL "1")
    list(APPEND SET_SDKCONFIG_DEFAULTS "sdkconfig_defaults/sdkconfig.light_sleep_gpio_on")
    message(STATUS "Appending sdkconfig.light_sleep_gpio_on to SET_SDKCONFIG_DEFAULTS")
else()
    list(APPEND SET_SDKCONFIG_DEFAULTS "sdkconfig_defaults/sdkconfig.light_sleep_gpio_off")
    message(STATUS "Appending sdkconfig.light_sleep_gpio_off to SET_SDKCONFIG_DEFAULTS")
endif()

# Create a new file with the name "sdkconfig.defaults"
set(SDKCONFIG_DEVICE_FILE "${CMAKE_SOURCE_DIR}/sdkconfig.defaults")

# Write the configurations to the file
file(WRITE ${SDKCONFIG_DEVICE_FILE} 
"#
# Automatically generated file; DO NOT EDIT.
# Espressif IoT Development Framework (ESP-IDF) Configuration
#
")

# Append each SET_SDKCONFIG_DEFAULTS file to the sdkconfig.defaults file
foreach(SDKCONFIG_DEFAULTS ${SET_SDKCONFIG_DEFAULTS})
    file(READ ${SDKCONFIG_DEFAULTS} SDKCONFIG_CONTENT)
    file(APPEND ${SDKCONFIG_DEVICE_FILE} "\n# Contents of ${SDKCONFIG_DEFAULTS}\n${SDKCONFIG_CONTENT}")
endforeach()

message(STATUS "Created sdkconfig.defaults with default configurations")


## KEEP ALIVE
add_definitions(-DED_KEEP_ALIVE=4000)

## OTA MACROS
add_definitions(-DESP_OTA_CLIENT_ENDPOINT=10)
# Define HEX 0x0101 or DEC 257
set(HW_VERSION "257")
set(VERSION "16843022")
set(MANUFACTURER "4097")
add_definitions(-DOTA_UPGRADE_HW_VERSION=${HW_VERSION})
add_definitions(-DOTA_UPGRADE_DOWNLOADED_FILE_VERSION=${VERSION})
add_definitions(-DOTA_UPGRADE_RUNNING_FILE_VERSION=${VERSION})
add_definitions(-DOTA_UPGRADE_IMAGE_TYPE=${MODEL_ID_DECIMAL})
add_definitions(-DOTA_UPGRADE_MANUFACTURER=${MANUFACTURER})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(FILE_NAME "device_${MODEL_ID_FORMATED}_v${VERSION}${DEV_PROD}")
project(${FILE_NAME})

#### ###################### ####
#### Create OTA-Update File ####
set(MY_DIR ${CMAKE_SOURCE_DIR}/ota)
file(MAKE_DIRECTORY ${MY_DIR})

set(PYTHON_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/create_ota.py)
add_custom_target(RunPostBuildScript ALL
    COMMAND ${CMAKE_COMMAND} -E echo  "Running Python script..."
    COMMAND ${CMAKE_COMMAND} -E env python3 ${PYTHON_SCRIPT} -m ${MANUFACTURER} -i ${MODEL_ID_DECIMAL} -v ${VERSION} ${CMAKE_BINARY_DIR}/${FILE_NAME}.bin > ${CMAKE_SOURCE_DIR}/ota/${FILE_NAME}.ota
)
add_dependencies(RunPostBuildScript ${FILE_NAME}.elf)
#### Create OTA-Update File ####
#### ###################### ####