/****************************************************************
 * Copyright (c) 2025-2026, Myriota Pty Ltd, All Rights Reserved
 * @file  main.c
 * @brief Entry point for application
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
#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <app_version.h>

#include "git_version.h"
#include "hyperpulse_lib.h"
#include "modem_at_host.h"
#include "modem.h"

LOG_MODULE_REGISTER(serial_ntn_modem, LOG_LEVEL_INF);

int main(void)
{
	printk("HyperPulse AT Modem Application: v%s\n", APP_VERSION_STRING);

	// Initialise HyperPulse library
	int err = hyperpulse_lib_init();
	if (err != 0) {
		printk("Failed to initialise HyperPulse library (err: %d)", err);
		return 0;
	}

	err = modem_enable_downlink_message_notification();
	if (err != 0) {
		printk("Failed to enable downlink message notifications (err: %d)", err);
	}

	// Initialise AT Host
	err = modem_at_host_init();
	if (err != 0) {
		printk("Failed to initialise AT host (err: %d)", err);
		return 0;
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
