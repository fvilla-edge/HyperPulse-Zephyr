/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  configuration.h
 * @brief Application configuration interface
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
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup app_config Application Configuration API
 * @brief Functions to initialize and retrieve application configuration parameters.
 * @{
 */

/**
 * @brief Initialize the configuration module.
 *
 * Must be called before any other configuration functions.
 *
 * @return 0 if succeeded, negative value if failed.
 */
int config_init(void);

/**
 * @brief Get the set configured time between transmitted messages.
 *
 * @return Message period in seconds.
 */
uint32_t config_get_uplink_message_period(void);

/**
 * @brief Check if GNSS fix is enabled for messages.
 *
 * @return true if GNSS fix is enabled, false otherwise.
 */
bool config_get_enable_gnss_state(void);

/** @} */ // end of app_config

#endif /* CONFIGURATION_H */
