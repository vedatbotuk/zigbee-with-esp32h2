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

#include <stdint.h>
#include "esp_zigbee_core.h"
#include "esp_log.h"

static const char *TAG_ZB_UPDATE_WATER = "UPDATE_WATERLEAK_CLUSTER";

void zb_update_waterleak(uint16_t leak)
{
    esp_zb_lock_acquire(portMAX_DELAY);
    esp_zb_zcl_status_t state = esp_zb_zcl_set_attribute_val(
        DEVICE_ENDPOINT,
        ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID,
        &leak,
        false);
    esp_zb_lock_release();

    /* Check for error */
    if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
    {
        ESP_LOGE(TAG_ZB_UPDATE_WATER, "Setting waterleak attribute failed with %x", state);
    }
}

void zb_report_waterleak(uint16_t leak)
{
    static esp_zb_zcl_ias_zone_status_change_notif_cmd_t waterleak_chg_not_cmd = {};
    waterleak_chg_not_cmd.zcl_basic_cmd.src_endpoint = DEVICE_ENDPOINT;
    waterleak_chg_not_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    waterleak_chg_not_cmd.zone_status = leak;
    waterleak_chg_not_cmd.zone_id = ESP_ZB_ZCL_CMD_IAS_ZONE_ZONE_STATUS_CHANGE_NOT_ID;
    waterleak_chg_not_cmd.delay = 1;

    /* Request sending */
    esp_zb_lock_acquire(portMAX_DELAY);
    esp_zb_zcl_ias_zone_status_change_notif_cmd_req(&waterleak_chg_not_cmd);
    esp_zb_lock_release();
}
