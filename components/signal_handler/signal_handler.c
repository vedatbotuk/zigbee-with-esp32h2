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
#include "esp_zigbee_core.h"
#include "esp_check.h"
#ifdef DEEP_SLEEP
#include "deep_sleep.h"
#endif

const char *TAG_SIGNAL_HANDLER = "SIGNAL";
bool conn = false;

#ifdef MIX_SLEEP
uint8_t deepsleep_cnt = 0;
#endif

void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG_SIGNAL_HANDLER, "Failed to start Zigbee bdb commissioning");
}

bool connection_status()
{
    return conn;
}

#ifdef MIX_SLEEP
void deep_sleep_check()
{
    if (deepsleep_cnt >= 10)
    {
        deepsleep_cnt = 0;
        /* Start the one-shot timer */
        ESP_LOGI(TAG_SIGNAL_HANDLER, "Start one-shot timer for %ds to enter the deep sleep", before_deep_sleep_time_sec);
        start_deep_sleep();
    }
    else
    {
        deepsleep_cnt += 1;
        ESP_LOGI(TAG_SIGNAL_HANDLER, "deep-sleep counter for not connecting: %d", deepsleep_cnt);
    }
}
#endif

void create_signal_handler(esp_zb_app_signal_t signal_struct)
{
    uint32_t *p_sg_p = signal_struct.p_app_signal;
    esp_err_t err_status = signal_struct.esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    esp_zb_zdo_signal_leave_params_t *leave_params = NULL;
    switch (sig_type)
    {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        conn = false;
        ESP_LOGI(TAG_SIGNAL_HANDLER, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK)
        {
            ESP_LOGI(TAG_SIGNAL_HANDLER, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new())
            {
                ESP_LOGI(TAG_SIGNAL_HANDLER, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            }
            else
            {
                conn = true;
#ifdef DEEP_SLEEP
                /* Start the one-shot timer */
                ESP_LOGI(TAG_SIGNAL_HANDLER, "Start one-shot timer for %ds to enter the deep sleep", before_deep_sleep_time_sec);
                start_deep_sleep();
#endif
                ESP_LOGI(TAG_SIGNAL_HANDLER, "Device rebooted");
            }
        }
        else
        {
            /* commissioning failed */
            conn = false;
            ESP_LOGW(TAG_SIGNAL_HANDLER, "Failed to initialize Zigbee stack (status: %d)", err_status);
#ifdef MIX_SLEEP
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
            deep_sleep_check();
#endif
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK)
        {
            conn = true;
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG_SIGNAL_HANDLER, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
#ifdef DEEP_SLEEP
            /* Start the one-shot timer */
            ESP_LOGI(TAG_SIGNAL_HANDLER, "Start one-shot timer for %ds to enter the deep sleep", before_deep_sleep_time_sec);
            start_deep_sleep();
#endif
        }
        else
        {
            conn = false;
            ESP_LOGI(TAG_SIGNAL_HANDLER, "Network steering was not successful (status: %d)", err_status);
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
#ifdef MIX_SLEEP
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
            deep_sleep_check();
#endif
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        leave_params = (esp_zb_zdo_signal_leave_params_t *)esp_zb_app_signal_get_params(p_sg_p);
        if (leave_params->leave_type == ESP_ZB_NWK_LEAVE_TYPE_RESET)
        {
            ESP_LOGI(TAG_SIGNAL_HANDLER, "Reset device");
            esp_zb_factory_reset();
        }
        break;
#ifdef LIGHT_SLEEP
    case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
        ESP_LOGI(TAG_SIGNAL_HANDLER, "Zigbee can sleep");
        esp_zb_sleep_now();
        break;
#ifdef ROUTER_DEVICE
    case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
        if (err_status == ESP_OK)
        {
            if (*(uint8_t *)esp_zb_app_signal_get_params(p_sg_p))
            {
                ESP_LOGI(TAG, "Network(0x%04hx) is open for %d seconds", esp_zb_get_pan_id(), *(uint8_t *)esp_zb_app_signal_get_params(p_sg_p));
            }
            else
            {
                ESP_LOGW(TAG, "Network(0x%04hx) closed, devices joining not allowed.", esp_zb_get_pan_id());
            }
        }
        break;
#endif
#endif
    default:
// TODO: BUG When no sleep implemented will printed the following log the whole time.
#ifdef LIGHT_SLEEP
        ESP_LOGI(TAG_SIGNAL_HANDLER, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type, esp_err_to_name(err_status));
#endif
        break;
    }
}