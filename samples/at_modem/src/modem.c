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

int32_t modem_enable_downlink_message_notification(void)
{
	// Enable downlink message notification unsolicited result code (URC).
	return send_at_command_check_ok("AT#MRECV=1");
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
