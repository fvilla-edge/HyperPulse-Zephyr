/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  periodic_uplink.h
 * @brief Manage periodic uplink message scheduling work
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
#ifndef PERIODIC_UPLINK_H
#define PERIODIC_UPLINK_H

#include <stdint.h>

/**
 * @defgroup periodic_uplink Periodic Uplink
 * @brief API for managing periodic uplink message scheduling
 * @{
 */

/**
 * @brief Initialise and start the periodic uplink message scheduler.
 *
 * Once started, the scheduler will begin sending uplink messages at the
 * configured interval. This function is called once during system
 * initialization.
 */
void periodic_uplink_init_and_start(void);

/**
 * @brief Stop the periodic uplink message scheduler.
 *
 * Halts any ongoing uplink message scheduling and prevents further messages
 * from being sent until the uplink message scheduler is restarted.
 *
 * @return 0 if succeeded, negative value if failed
 */
int periodic_uplink_stop(void);

/**
 * @brief Update the periodic uplink message scheduler with a new period.
 *
 * The new period takes effect immediately for subsequent messages after
 * the function is called.
 *
 * @param[in] new_period Period for uplink message scheduling, in seconds
 * @return 0 if succeeded, negative value if failed
 */
int periodic_uplink_update_period(const uint32_t new_period);

/** @} */ // end of periodic_uplink group

#endif /* PERIODIC_UPLINK_H */
