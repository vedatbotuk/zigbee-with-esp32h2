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

#include "esp_log.h"
#include "light_on_off.h"
#include "driver/gpio.h"

void light_driver_set_power(bool power)
{
  gpio_set_level(GPIO_OUTPUT_PIN, power ? 1 : 0);
}

void light_driver_init(bool power)
{
  // GPIO configuration for an output
  gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,       // No interrupts for the pin
      .mode = GPIO_MODE_OUTPUT,             // Set pin as output
      .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,  // Configure the desired pin
      .pull_down_en = GPIO_PULLDOWN_ENABLE, // Enable pull-down
      .pull_up_en = GPIO_PULLUP_DISABLE     // Disable pull-up
  };

  gpio_config(&io_conf);               // Apply the configuration
  gpio_sleep_sel_dis(GPIO_OUTPUT_PIN); // Disable sleep select
}