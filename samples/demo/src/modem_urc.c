/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  modem_urc.c
 * @brief Handle unsolicited modem events (URCs)
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

#include <stdio.h>

#include "app_gnss.h"
#include "downlink_num.h"
#include "modem.h"

LOG_MODULE_REGISTER(modem_urc, LOG_LEVEL_INF);

typedef struct {
	const char *const type_str;           ///< URC Response type
	process_urc_response_handler handler; /// Function pointer to process the URC response.
} process_urc_response_entry_t;

// Handler to process Location URC Modem responses
static void process_urc_location(const char *const response)
{
	// Hold the location info from the location response.
	modem_location_info_t location = {0};

	int err = modem_parse_location_response(response, &location);
	if (err == 0) {
		// Deliver parsed location to app_gnss via API
		if (app_gnss_push_location_to_queue(&location) != 0) {
			LOG_DBG("Failed to push location info to queue");
		}
	} else {
		LOG_ERR("Failed to parse location URC modem response (%d)", err);
	}
	return;
}

// Handler to process Message Received URC Modem responses.
static void process_urc_message_received(const char *const response)
{
	// The size of an incomimg downlink message size.
	uint32_t downlink_message_size = 0;

	// Buffer to store an incoming downlink message in hexadecimal format.
	char downlink_message_hex[MODEM_DOWNLINK_MSG_HEX_SIZE] = {0};

	int err =
		modem_get_downlink_message(response, downlink_message_hex, &downlink_message_size);
	if (err == 0) {
		unsigned int downlink_num = 0;
		if (downlink_message_size > 0 && sscanf(downlink_message_hex, "%2x", &downlink_num) == 1) {
			downlink_num_set((uint8_t)downlink_num);
			LOG_INF("Updated num field from DL byte: %u", downlink_num);
		}

		LOG_INF("Received DL message (len = %d bytes) : 0x%s\n", downlink_message_size,
			downlink_message_hex);
	} else {
		LOG_ERR("Failed to get downlink message (%d)", err);
	}
	return;
}

// Lookup table for processing URC Modem responses.
const process_urc_response_entry_t urc_response_handlers[] = {
	{"#MLOCATION:", process_urc_location},
	{"#MRECV:", process_urc_message_received},
};

#define URC_HANDLERS_TABLE_SIZE (sizeof(urc_response_handlers) / sizeof(urc_response_handlers[0]))

// APP handler to process URC Modem responses is defined by the Application.
void hyperpulse_lib_unsolicited_at_response_app_handler(const char *const response)
{
	for (int i = 0; i < URC_HANDLERS_TABLE_SIZE; i++) {
		int type_str_len = strlen(urc_response_handlers[i].type_str);
		if (strncmp(response, urc_response_handlers[i].type_str, type_str_len) == 0) {
			return urc_response_handlers[i].handler(response);
		}
	}

	LOG_WRN("Unknow URC Modem Response: %s", response);
	return;
}
