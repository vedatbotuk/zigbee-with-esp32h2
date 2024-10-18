/*
 * selforganized_802.15.4_network_with_esp32
 * Copyright (c) 2024 Vedat Botuk.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "macros.h"
#include "device.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "esp_log.h"
#include "update_cluster.h"
#include "create_cluster.h"
#include "signal_handler.h"
// #include "pump_switch.h"
#include "light_on_off.h"

#ifdef OTA_UPDATE
#include "ota.h"
#endif

#ifdef LIGHT_SLEEP
#include "light_sleep.h"
#endif

static char firmware_version[16] = {7, 'v', 'e', 'r', '0', '.', '1', '8'};
static const char *TAG = "DEVICE";

/********************* Define functions **************************/
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    create_signal_handler(*signal_struct);
}

#if defined AUTOMATIC_IRRIGATION || defined LIGHT_ON_OFF
static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;
    bool light_state = 0;

    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);
    if (message->info.dst_endpoint == DEVICE_ENDPOINT)
    {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF)
        {
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL)
            {
                light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
                ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
                light_driver_set_power(light_state);
            }
        }
    }
    return ret;
}
#endif

#if defined OTA_UPDATE || defined AUTOMATIC_IRRIGATION || defined LIGHT_ON_OFF
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id)
    {
#ifdef OTA_UPDATE
    case ESP_ZB_CORE_OTA_UPGRADE_VALUE_CB_ID:
        ret = zb_ota_upgrade_status_handler(*(esp_zb_zcl_ota_upgrade_value_message_t *)message);
        break;
#endif
#if defined AUTOMATIC_IRRIGATION || defined LIGHT_ON_OFF
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
#endif
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}
#endif

static void esp_zb_task(void *pvParameters)
{
    /* initialize Zigbee stack */
#ifdef END_DEVICE
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    ESP_LOGI(TAG, "Enable END_DEVICE");
#endif
#ifdef ROUTER_DEVICE
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
    ESP_LOGI(TAG, "Enable ROUTER_DEVICE");
#endif
    /* The order in the following 3 lines must not be changed. */
#ifdef LIGHT_SLEEP
    sleep_enable();
#endif
    esp_zb_init(&zb_nwk_cfg);
#ifdef LIGHT_SLEEP
    sleep_configure();
    ESP_LOGI(TAG, "Enable LIGHT_SLEEP");
#endif
#ifdef ROUTER_DEVICE
    esp_zb_set_tx_power(10);
#endif
#ifdef END_DEVICE
    // TODO: Set the TX power for the end device only for the test
    esp_zb_set_tx_power(0);
#endif

    /* create cluster lists for this endpoint */
    esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

    create_basic_cluster(esp_zb_cluster_list, firmware_version);
    create_identify_cluster(esp_zb_cluster_list);
#ifdef SENSOR_TEMPERATURE
    create_temp_cluster(esp_zb_cluster_list);
    ESP_LOGI(TAG, "Create SENSOR_TEMPERATURE Cluster");

#endif
#ifdef SENSOR_HUMIDITY
    create_hum_cluster(esp_zb_cluster_list);
    ESP_LOGI(TAG, "Create SENSOR_HUMIDITY Cluster");

#endif
#ifdef SENSOR_WATERLEAK
    create_waterleak_cluster(esp_zb_cluster_list);
    ESP_LOGI(TAG, "Create SENSOR_WATERLEAK Cluster");

#endif
#ifdef BATTERY
    create_battery_cluster(esp_zb_cluster_list);
    ESP_LOGI(TAG, "Create BATTERY Cluster");

#endif
#ifdef OTA_UPDATE
    create_ota_cluster(esp_zb_cluster_list);
    ESP_LOGI(TAG, "Create OTA_UPDATE Cluster");
#endif
#ifdef AUTOMATIC_IRRIGATION
    create_water_pump_switch_cluster(esp_zb_cluster_list);
#endif
#ifdef LIGHT_ON_OFF
    create_light_switch_cluster(esp_zb_cluster_list);
#endif

    esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = DEVICE_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEST_DEVICE_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list, endpoint_config);

    esp_zb_device_register(esp_zb_ep_list);
#if defined OTA_UPDATE || defined AUTOMATIC_IRRIGATION || defined LIGHT_ON_OFF
    esp_zb_core_action_handler_register(zb_action_handler);
#endif
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

#if defined AUTOMATIC_IRRIGATION || defined LIGHT_ON_OFF
static esp_err_t deferred_driver_init(void)
{
    light_driver_init(LIGHT_DEFAULT_OFF);
    return ESP_OK;
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "--- Application Start ---");
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
#ifdef LIGHT_SLEEP
    ESP_ERROR_CHECK(esp_zb_power_save_init());
#endif
#ifdef LIGHT_ON_OFF
    ESP_LOGI(TAG_SIGNAL_HANDLER, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
#endif
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
