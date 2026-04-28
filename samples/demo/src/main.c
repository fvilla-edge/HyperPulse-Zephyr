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
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/printk.h>

#include <stdlib.h>

#include <app_version.h>
#include "git_version.h"

#include "app_gnss.h"
#include "configuration.h"
#include "hardware_controls.h"
#include "hyperpulse_lib.h"
#include "modem.h"
#include "periodic_uplink.h"

LOG_MODULE_REGISTER(demo_app, LOG_LEVEL_INF);

static void uart_test_mode_listen_forever(void)
{
	const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
	uint8_t rx_char;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART test mode unavailable: uart1 is not ready");
		return;
	}

	printk("UART test mode active on %s (FTDI). Waiting for RX bytes...\n", uart_dev->name);

	for (;;) {
		if (uart_poll_in(uart_dev, &rx_char) == 0) {
			if (rx_char == '\r' || rx_char == '\n') {
				printk("\n");
				continue;
			}

			printk("RX: 0x%02X '%c'\n", rx_char, rx_char);
		} else {
			k_sleep(K_MSEC(10));
		}
	}
}

int main(void)
{
	printk("Demo Application: v%s\n", APP_VERSION_STRING);

	// Check for successful HyperPulse library initialisation at start of main
	// since CONFIG_HYPERPULSE_AUTO_LIB_INIT is set to y
	if (!hyperpulse_lib_is_initialised()) {
		LOG_ERR("HyperPulse library failed to initialise");
		return 0;
	}

	// Initialise configuration
	config_init();

	// Initialise hardware control
	hardware_control_init();

	// Signal initialising complete
	hardware_control_flash_led(LED_1, 1);

	// Permanent UART test mode: keep listening and do not continue to GNSS.
	uart_test_mode_listen_forever();

	// Start shell after configuration has been initialised
	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL)) {
		shell_start(shell_backend_uart_get_ptr());
	}

	// Wait for an initial valid GNSS fix before starting the message scheduler
	app_gnss_wait_for_valid_fix();

	printk("HOLU\n");

	modem_enable_downlink_message_notification();

	// Start the periodic uplink message scheduler
	periodic_uplink_init_and_start();

	// Signal message scheduler start
	hardware_control_flash_led(LED_1, 2);

	for (;;) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

static void sh_print_app_version(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);
	shell_print(sh, "%s", APP_VERSION_STRING);
	return;
}

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_app,
	SHELL_CMD(version, NULL, "print application version", sh_print_app_version),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
/* clang-format on */

SHELL_CMD_REGISTER(app, &sub_app, "Application commands", NULL);
