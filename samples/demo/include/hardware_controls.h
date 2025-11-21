/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  hardware_controls.h
 * @brief Interface for hardware control.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Attribution
 *
 * This file is licensed under the BSD with attribution  (the "License"); you
 * may not use these files except in compliance with the License.
 *
 * You may obtain a copy of the License here:
 * LICENSE-BSD-3-Clause-Attribution.txt and at
 * https://spdx.org/licenses/BSD-3-Clause-Attribution.html
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************/
#ifndef HARDWARE_CONTROLS_H
#define HARDWARE_CONTROLS_H

/**
 * @defgroup hardware_controls Hardware Controls
 * @brief API for controlling LEDs and other hardware peripherals.
 * @{
 */

/**
 * @brief LED identifiers
 */
typedef enum {
	LED_1 = 0, /**< LED 1 */
	LED_2,     /**< LED 2 */
} led_id_t;

/**
 * @brief Flash an LED a specified number of times.
 *
 * @param[in] led_id ID of the LED to flash (from led_id_t)
 * @param[in] count Number of times to flash the LED
 * @return 0 if succeeded, negative value if failed
 */
int hardware_control_flash_led(const led_id_t led_id, const int count);

/**
 * @brief Initialize hardware controls.
 *
 * Sets up any necessary hardware peripherals.
 */
void hardware_control_init(void);

/** @} */ // end of hardware_controls group

#endif /* HARDWARE_CONTROLS_H */
