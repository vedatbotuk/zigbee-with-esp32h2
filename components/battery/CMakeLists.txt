idf_component_register(
    SRCS battery_read.c
    INCLUDE_DIRS .
    PRIV_REQUIRES esp_adc update_cluster espressif__esp-zigbee-lib driver
)
