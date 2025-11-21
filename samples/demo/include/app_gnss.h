/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  app_gnss.h
 * @brief Handle GNSS fix acquisition and related AT commands.
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
#ifndef APP_GNSS_H
#define APP_GNSS_H

#include "modem.h"

/**
 * @defgroup app_gnss GNSS Application Interface
 * @brief Functions to acquire and handle GNSS fixes using the HyperPulse library.
 *
 * This module provides functions to wait for a valid GNSS fix, retrieve
 * fix data, and queue location updates received via the modem's URC layer.
 * @{
 */

/**
 * @brief Block until a valid GNSS fix is obtained.
 *
 * This function is used at startup to ensure the system
 * has a valid location before proceeding.
 */
void app_gnss_wait_for_valid_fix(void);

/**
 * @brief Obtain the current GNSS fix (position and timestamp).
 *
 * @param[out] location Pointer to a struct to store the fix information.
 * @return 0 if succeeded, negative value if failed.
 */
int app_gnss_get_fix(modem_location_info_t *const location);

/**
 * @brief Called by the Modem URC layer when a parsed location is available.
 *
 * This function will queue the new location for the application to consume.
 *
 * @param[in] location Pointer to the location struct containing the new fix.
 * @return 0 if succeeded, negative value if failed.
 */
int app_gnss_push_location_to_queue(const modem_location_info_t *const location);

/** @} */ // end of app_gnss

#endif /* APP_GNSS_H */
