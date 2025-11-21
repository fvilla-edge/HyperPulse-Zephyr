/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  modem.c
 * @brief Helper functions for interacting with the HyperPulse library.
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

#include "modem.h"
#include "hyperpulse_lib.h"

LOG_MODULE_REGISTER(modem, LOG_LEVEL_INF);

static int send_at_command_check_ok(const char *cmd)
{
	char response[MODEM_AT_CMD_RESPONSE_BUF_SIZE] = {0};
	hyperpulse_lib_send_at_command(cmd, response, sizeof(response));

	if (strstr(response, "OK") == NULL) {
		LOG_ERR("AT command failed: %s", cmd);
		return -EIO;
	}

	return 0;
}

int32_t modem_parse_location_response(const char *const response,
				      modem_location_info_t *const location)
{
	// Expecting: #MLOCATION: <method>,<lat>,<lon>,<elev>,<timestamp_s>,

	if (!response || !location || strncmp(response, "#MLOCATION:", 11) != 0) {
		return -EINVAL;
	}

	int32_t acc, lat, lon, elev;
	time_t timestamp;

	int parsed = sscanf(response, "#MLOCATION: %d,%d,%d,%d,%jd,", &acc, &lat, &lon, &elev,
			    &timestamp);
	if (parsed != 5) {
		return -EINVAL;
	}

	LOG_INF("#MLOCATION: acc=%d lat=%d, lon=%d, elev=%d, timestamp=%jd", acc, lat, lon, elev,
		timestamp);

	location->accuracy = acc;
	location->position.latitude = lat;
	location->position.longitude = lon;
	location->position.elevation_mm = elev;
	location->timestamp_s = timestamp;

	return 0;
}

int32_t modem_get_system_time_s(time_t *const time_s)
{
	if (!time_s) {
		return -EINVAL;
	}

	char response[MODEM_AT_CMD_RESPONSE_BUF_SIZE] = {0};
	if (hyperpulse_lib_send_at_command("AT#MTIME?", response, sizeof(response)) < 0) {
		return -EIO;
	}

	// Expecting: <current_time_ms> \rOK
	time_t time_ms = 0;
	int parsed = sscanf(response, "%jd \rOK", &time_ms);
	if (parsed != 1) {
		LOG_ERR("Failed to parse system time from response: %s", response);
		return -EINVAL;
	}

	*time_s = time_ms / 1000;
	LOG_DBG("System time = %llu s", *time_s);

	return 0;
}

int32_t modem_enable_downlink_message_notification(void)
{
	// Enable downlink message notification unsolicited result code (URC).
	return send_at_command_check_ok("AT#MRECV=1");
}

int modem_get_downlink_message(const char *const response, char *message_hex,
			       uint32_t *message_length)
{
	// Expecting #MRECV: <length>,"<downlink_message>"\r

	if (!response || !message_hex || strncmp(response, "#MRECV:", 7) != 0) {
		return -EINVAL;
	}

	uint32_t length = 0;
	char tmp_hex[MODEM_DOWNLINK_MSG_HEX_SIZE] = {0};

	char fmt[64] = "";
	snprintf(fmt, sizeof(fmt) - 1, "#MRECV: %%d,\"%%%d[^\"]", MODEM_DOWNLINK_MSG_HEX_SIZE);

	int parsed = sscanf(response, fmt, &length, tmp_hex);
	if (parsed != 2) {
		return -EINVAL;
	}

	if (length > MODEM_DOWNLINK_MSG_SIZE) {
		LOG_ERR("Downlink message length (%d) > buffer size (%d)", length,
			MODEM_DOWNLINK_MSG_SIZE);
		// the truncated downlink message is in message_hex.
		return -ENOBUFS;
	}

	strncpy(message_hex, tmp_hex, strlen(tmp_hex));
	*message_length = length;

	return 0;
}

int32_t modem_get_temperature_celsius(int8_t *const temperature_celsius)
{
	if (!temperature_celsius) {
		return -EINVAL;
	}

	char response[MODEM_AT_CMD_RESPONSE_BUF_SIZE] = {0};
	if (hyperpulse_lib_send_at_command("AT%XTEMP?", response, sizeof(response)) < 0) {
		LOG_ERR("Failed to send temperature AT command");
		return -EIO;
	}

	if (strstr(response, "OK") == NULL) {
		LOG_ERR("Unexpected response: %s", response);
		return -EIO;
	}

	if (sscanf(response, "%%XTEMP: %hhd", temperature_celsius) != 1) {
		return -EINVAL;
	}

	LOG_DBG("Temperature = %d°C", *temperature_celsius);
	return 0;
}

int32_t modem_get_battery_voltage_mv(uint16_t *const voltage_mv)
{
	if (!voltage_mv) {
		return -EINVAL;
	}

	char response[MODEM_AT_CMD_RESPONSE_BUF_SIZE] = {0};
	if (hyperpulse_lib_send_at_command("AT%XVBAT", response, sizeof(response)) < 0) {
		LOG_ERR("Failed to send voltage AT command");
		return -EIO;
	}

	if (strstr(response, "OK") == NULL) {
		LOG_ERR("Unexpected response: %s", response);
		return -EIO;
	}

	if (sscanf(response, "%%XVBAT: %hu", voltage_mv) != 1) {
		return -EINVAL;
	}

	LOG_DBG("Battery voltage = %u mV", *voltage_mv);
	return 0;
}

int32_t modem_schedule_uplink_message(const char *const uplink_msg, const size_t msg_len)
{
	if (!uplink_msg) {
		return -EINVAL;
	}

	if (msg_len > MODEM_UPLINK_MSG_SIZE) {
		LOG_ERR("Uplink message size (%d) > buffer size (%d)", msg_len,
			MODEM_UPLINK_MSG_SIZE);
		return -ENOBUFS;
	}

	char hex_sting[MODEM_UPLINK_MSG_HEX_SIZE] = {
		0}; // one byte is represented by two hexadecimal characters
	bin2hex(uplink_msg, msg_len, hex_sting, sizeof(hex_sting));

	char command[MODEM_AT_CMD_BUF_SIZE] = {0};
	snprintf(command, ARRAY_SIZE(command) - 1, "AT#MSEND=%zu,\"%s\"", msg_len, hex_sting);

	return send_at_command_check_ok(command);
}

int32_t modem_request_location(const modem_location_request_t request)
{
	char command[MODEM_AT_CMD_BUF_SIZE] = {0};

	snprintf(command, ARRAY_SIZE(command) - 1, "AT#MLOCATION?%u,%u,%u,%u", request.method,
		 request.accuracy, request.max_age_last_update_s, request.timeout_s);

	return send_at_command_check_ok(command);
}

// Initialization of AUX pin
#if defined(CONFIG_BOARD_CIRCUITDOJO_FEATHER_NRF9151) || \
	defined(CONFIG_BOARD_MYRIOTA_HYPERPULSE_DK_NRF9151_CIRCUITDOJO_NS)
#include <modem/nrf_modem_lib.h>

#define AUXANTCFG_ENABLE "AT\%XANTCFG=1"

NRF_MODEM_LIB_ON_INIT(aux_init_hook, on_modem_lib_init, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	ARG_UNUSED(ctx);

	if (ret != 0) {
		return;
	}

	printk("*** Setting antenna configuration: %s ***\n", AUXANTCFG_ENABLE);
	int err = send_at_command_check_ok(AUXANTCFG_ENABLE);
	if (err != 0) {
		LOG_ERR("Failed to set configuration (err: %d)", err);
	}
}
#endif
