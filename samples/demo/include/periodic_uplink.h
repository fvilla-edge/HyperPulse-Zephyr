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

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup periodic_uplink Periodic Uplink
 * @brief API for managing periodic uplink message scheduling
 * @{
 */

/**
 * @brief Initialise periodic uplink work queue and workers.
 */
void periodic_uplink_init(void);

/**
 * @brief Start the periodic uplink scheduler ticks.
 */
void periodic_uplink_start(void);

/**
 * @brief Initialise and start the periodic uplink message scheduler.
 *
 * Backward-compatible helper equivalent to init + start.
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

/**
 * @brief Send an uplink message immediately with custom num field.
 *
 * The message is populated using the same data path as periodic messages,
 * but the caller provides the value used in the `num` field.
 *
 * @param[in] num_value Value to set in message `num` field
 * @return 0 if succeeded, negative value if failed
 */
int periodic_uplink_send_now(uint8_t num_value);

/**
 * @brief Set whether a valid initial GNSS fix is available.
 *
 * When set to false, uplink messages are still generated but location/time
 * fields that depend on GNSS are populated with invalid placeholders.
 *
 * @param[in] ready True once initial GNSS fix is acquired
 */
void periodic_uplink_set_gnss_ready(bool ready);

/** @} */ // end of periodic_uplink group

#endif /* PERIODIC_UPLINK_H */
