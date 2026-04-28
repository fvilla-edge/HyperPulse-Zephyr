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

#include <errno.h>
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

#define UART_LISTENER_STACK_SIZE 1024
#define UART_LISTENER_PRIORITY   7

K_THREAD_STACK_DEFINE(uart_listener_stack, UART_LISTENER_STACK_SIZE);
static struct k_thread uart_listener_thread;

static int parse_uart_line_to_u8(const char *line, uint8_t *value)
{
	char *endptr = NULL;
	unsigned long parsed = strtoul(line, &endptr, 0);

	if (endptr == line || *endptr != '\0' || parsed > UINT8_MAX) {
		return -EINVAL;
	}

	*value = (uint8_t)parsed;
	return 0;
}

static void uart_line_listener(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
	uint8_t rx_char = 0;
	char line_buf[16] = {0};
	size_t line_idx = 0;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART listener unavailable: uart1 is not ready");
		return;
	}

	LOG_INF("UART uplink listener active on %s", uart_dev->name);

	for (;;) {
		if (uart_poll_in(uart_dev, &rx_char) != 0) {
			k_sleep(K_MSEC(10));
			continue;
		}

		if (rx_char == '\r' || rx_char == '\n') {
			if (line_idx == 0) {
				continue;
			}

			line_buf[line_idx] = '\0';

			uint8_t uplink_value = 0;
			if (parse_uart_line_to_u8(line_buf, &uplink_value) != 0) {
				LOG_WRN("Ignoring invalid UART line: '%s' (expected 0..255)", line_buf);
			} else {
				int err = periodic_uplink_send_now(uplink_value);
				if (err < 0) {
					LOG_ERR("Failed UART-triggered uplink (num=%u, err=%d)",
						uplink_value, err);
				} else {
					LOG_INF("UART-triggered uplink queued (num=%u, rc=%d)",
						uplink_value, err);
				}
			}

			line_idx = 0;
			continue;
		}

		if (line_idx < (sizeof(line_buf) - 1)) {
			line_buf[line_idx++] = (char)rx_char;
		} else {
			LOG_WRN("UART line too long, dropping");
			line_idx = 0;
		}
	}
}

static void uart_line_listener_start(void)
{
	k_thread_create(&uart_listener_thread, uart_listener_stack,
			K_THREAD_STACK_SIZEOF(uart_listener_stack), uart_line_listener, NULL, NULL,
			NULL, UART_LISTENER_PRIORITY, 0, K_NO_WAIT);
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

	// Start shell after configuration has been initialised
	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL)) {
		shell_start(shell_backend_uart_get_ptr());
	}

	// Start UART listener immediately so incoming manual data is captured
	// even while waiting for the first GNSS fix.
	periodic_uplink_set_gnss_ready(false);
	periodic_uplink_init();
	uart_line_listener_start();

	// Wait for an initial valid GNSS fix before starting the message scheduler
	app_gnss_wait_for_valid_fix();
	periodic_uplink_set_gnss_ready(true);

	printk("HOLU\n");

	modem_enable_downlink_message_notification();
	periodic_uplink_start();

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
