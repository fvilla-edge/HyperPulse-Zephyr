/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  periodic_uplink.c
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
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_gnss.h"
#include "configuration.h"
#include "downlink_num.h"
#include "hardware_controls.h"
#include "modem.h"
#include "periodic_uplink.h"

LOG_MODULE_REGISTER(periodic_uplink, LOG_LEVEL_INF);

#define INVALID_LATITUDE  -900000000
#define INVALID_LONGITUDE 1800000000
#define INVALID_ELEVATION INT16_MIN
#define INVALID_TIME      0
#define INVALID_HUMIDITY_PERCENT 255U
#define DEMO_HUMIDITY_PERCENT    65
#define DEMO_NUM_VALUE           15U

#define UPLINK_WORK_STACK_SIZE 5120
#define UPLINK_WORK_PRIORITY   5
K_THREAD_STACK_DEFINE(uplink_work_stack, UPLINK_WORK_STACK_SIZE);

static struct k_work_q uplink_work_q;
static struct k_work_delayable periodic_uplink_work;
static bool uplink_work_initialised = false;

// Uplink message structure.
struct uplink_message_t {
	uint32_t sequence_number;    // Message count
	uint32_t time;               // Message scheduled time in epoch seconds
	int32_t latitude;            // Latitude scaled by 1e7
	int32_t longitude;           // Longitude scaled by 1e7
	int16_t elevation_m;         // Elevation in meters
	int8_t temperature_celsius;  // Internal temperature reading in degrees celsius
	uint16_t battery_voltage_mv; // Battery voltage in milli-volt
	uint8_t humidity_percent;    // Relative humidity in percent (demo fixed value)
	uint8_t num;                 // Fixed demo field
} __attribute__((packed));

static void printk_buffer_hex(const char *buf, size_t len)
{
	for (int i = 0; i < len; ++i) {
		printk("%02X", buf[i]);
	}
	printk("\n");
}

static uint32_t next_sequence_number(void)
{
	static uint32_t sequence_number = 0;
	return sequence_number++;
}

static void populate_uplink_message(struct uplink_message_t *msg)
{
	// Set sequence number
	msg->sequence_number = next_sequence_number();

	const bool gnss_fix_enable = config_get_enable_gnss_state();
	const bool humidity_enable = config_get_enable_humidity_state();

	// Update location
	modem_location_info_t location = {0};
	int err = app_gnss_get_fix(&location);
	if (err == 0) {
		msg->latitude = location.position.latitude;
		msg->longitude = location.position.longitude;
		msg->elevation_m = location.position.elevation_mm / 1000; // mm -> m

		// Populate message with time from GNSS fix if gnss_fix is enabled
		if (gnss_fix_enable) {
			msg->time = location.timestamp_s;
		}

	} else {
		LOG_ERR("Failed to get location");
		msg->latitude = INVALID_LATITUDE;
		msg->longitude = INVALID_LONGITUDE;
		msg->elevation_m = INVALID_ELEVATION;
		msg->time = INVALID_TIME;
	}

	// Populate message with system time if gnss_fix is disabled
	if (!gnss_fix_enable) {
		time_t system_time = 0;
		if (modem_get_system_time_s(&system_time) == 0) {
			msg->time = system_time;
		} else {
			LOG_ERR("Failed to get system time");
			msg->time = 0;
		}
	}

	// Update temperature
	int8_t temperature = 0;
	if (modem_get_temperature_celsius(&temperature) == 0) {
		msg->temperature_celsius = temperature;
	} else {
		LOG_ERR("Failed to get temperature");
		msg->temperature_celsius = INT8_MIN;
	}

	// Update battery voltage
	uint16_t voltage = 0;
	if (modem_get_battery_voltage_mv(&voltage) == 0) {
		msg->battery_voltage_mv = voltage;
	} else {
		LOG_ERR("Failed to get battery voltage");
		msg->battery_voltage_mv = 0;
	}

	// Populate humidity with a fixed demo value when enabled.
	msg->humidity_percent =
		humidity_enable ? DEMO_HUMIDITY_PERCENT : INVALID_HUMIDITY_PERCENT;
	msg->num = DEMO_NUM_VALUE;
	(void)downlink_num_get(&msg->num);

	return;
}

// Periodic Uplink Message Handler to schedule messages for the network.
static void periodic_uplink_work_handler(struct k_work *work)
{
	int32_t start_time_s = k_uptime_seconds();

	// Signal activity
	hardware_control_flash_led(LED_1, 1);

	// Puuplate uplink message
	struct uplink_message_t msg = {0};
	populate_uplink_message(&msg);

	LOG_INF("Scheduled uplink message: %u %u %d %d %d %hhd %u %u %u\n", msg.sequence_number,
		msg.time, msg.latitude, msg.longitude, msg.elevation_m, msg.temperature_celsius,
		msg.battery_voltage_mv, msg.humidity_percent, msg.num);

	printk("Scheduled uplink message (hex): 0x");
	printk_buffer_hex((const char *)&msg, sizeof(msg));

	// Schedule uplink message
	int err = modem_schedule_uplink_message((const char *)&msg, sizeof(msg));
	if (err != 0) {
		LOG_ERR("Failed to schedule uplink message (sequence_number: %d) (err: %d)",
			msg.sequence_number, err);
	}

	// Schedule next uplink message work
	uint32_t period = config_get_uplink_message_period();
	if (period > 0) {
		const uint32_t elapsed_time_s = k_uptime_seconds() - start_time_s;
		const uint32_t delay_s = (elapsed_time_s < period) ? (period - elapsed_time_s) : 0;
		k_work_reschedule_for_queue(&uplink_work_q, &periodic_uplink_work,
					    K_SECONDS(delay_s));
	}

	return;
}

int periodic_uplink_update_period(const uint32_t new_period)
{
	if (!uplink_work_initialised) {
		// The new period will be applied when the uplink work is run.
		return 0;
	}

	return k_work_reschedule_for_queue(&uplink_work_q, &periodic_uplink_work,
					   K_SECONDS(new_period));
}

int periodic_uplink_stop(void)
{
	return k_work_cancel_delayable(&periodic_uplink_work);
}

void periodic_uplink_init_and_start(void)
{
	k_work_queue_start(&uplink_work_q, uplink_work_stack,
			   K_THREAD_STACK_SIZEOF(uplink_work_stack), UPLINK_WORK_PRIORITY, NULL);

	k_work_init_delayable(&periodic_uplink_work, periodic_uplink_work_handler);

	k_work_reschedule_for_queue(&uplink_work_q, &periodic_uplink_work, K_SECONDS(0));

	uplink_work_initialised = true;
}
