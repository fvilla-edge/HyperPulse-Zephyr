/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  app_gnss.c
 * @brief GNSS fix acquisition and related functions.
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
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_gnss.h"
#include "hardware_controls.h"
#include "configuration.h"

LOG_MODULE_REGISTER(app_gnss, LOG_LEVEL_INF);

#define GNSS_FIX_CHECK_RETRY_SECS     10
#define LOCATION_UPDATE_TIMEOUT_SECS  360
#define LOCATION_REQUEST_TIMEOUT_SECS 90
#define LOCATION_MSGQ_TIMEOUT_MS      5000

// Queue size 1: store only the latest location.
K_MSGQ_DEFINE(location_info_queue, sizeof(modem_location_info_t), 1, 1);

int app_gnss_push_location_to_queue(const modem_location_info_t *const location)
{
	if (!location) {
		return -EINVAL;
	}

	const int64_t start_time = k_uptime_get();

	while (k_msgq_put(&location_info_queue, location, K_NO_WAIT) != 0) {
		// message queue is full: purge old data & try again
		k_msgq_purge(&location_info_queue);

		if ((k_uptime_get() - start_time) >= LOCATION_MSGQ_TIMEOUT_MS) {
			LOG_ERR("Failed to push location info in the queue");
			return -ETIMEDOUT;
		}

		k_msleep(1);
	}

	return 0;
}

void app_gnss_wait_for_valid_fix(void)
{
	const modem_location_request_t request = {.method = MODEM_LOCATION_METHOD_GNSS,
						  .accuracy = MODEM_LOCATION_ACCURACY_LOW,
						  .max_age_last_update_s = 0,
						  .timeout_s = LOCATION_REQUEST_TIMEOUT_SECS};

	// Before requesting location info from modem, empty location info queue.
	k_msgq_purge(&location_info_queue);

	if (modem_request_location(request) != 0) {
		LOG_ERR("Failed to request location info from modem");
		return;
	}

	// Wait until Modem URC handler pushes a location info into the queue.
	modem_location_info_t location = {0};
	while (1) {
		if (k_msgq_get(&location_info_queue, &location, K_SECONDS(1)) == 0) {
			// A GNSS fix has been obtained.
			break;
		} else {
			// Signal waiting for a GNSS fix.
			hardware_control_flash_led(LED_2, GNSS_FIX_CHECK_RETRY_SECS);
		}

		k_msleep(10);
	}

	return;
}

int app_gnss_get_fix(modem_location_info_t *const location)
{
	if (!location) {
		return -EINVAL;
	}

	modem_location_request_t request;
	request.method = MODEM_LOCATION_METHOD_GNSS;
	request.timeout_s = LOCATION_REQUEST_TIMEOUT_SECS;

	if (config_get_enable_gnss_state()) {
		// Request a new valid GNSS fix.
		request.accuracy = MODEM_LOCATION_ACCURACY_HIGH;
		request.max_age_last_update_s = 0;
	} else {
		// Allow reuse of the last valid GNSS fix by setting a large max age value.
		// This minimizes power consumption by avoiding a new GNSS acquisition.
		request.accuracy = MODEM_LOCATION_ACCURACY_LOW;
		request.max_age_last_update_s = UINT32_MAX;
	}

	// Before requesting location info from modem, empty location info queue.
	k_msgq_purge(&location_info_queue);

	if (modem_request_location(request) != 0) {
		LOG_ERR("Failed to request location info from modem");
		return -1;
	}

	// Wait until URC handler pushes a location info into the queue.
	int err =
		k_msgq_get(&location_info_queue, location, K_SECONDS(LOCATION_UPDATE_TIMEOUT_SECS));
	if (err != 0) {
		LOG_ERR("Failed to read location info from queue (%d)", err);
		return err;
	}

	return 0;
}
