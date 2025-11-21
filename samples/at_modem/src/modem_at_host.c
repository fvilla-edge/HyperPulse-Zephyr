/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  modem_at_host.c
 * @brief AT Host Interface for the HyperPulse library
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
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include "hyperpulse_lib.h"
#include "modem_at_host.h"

#define MODEM_AT_HOST_UART_INIT_TIMEOUT_MS 500
#define MODEM_AT_HOST_CMD_MAX_LEN          600
#define MODEM_AT_HOST_THREAD_PRIO          10
#define MODEM_AT_HOST_STACK_SIZE           5210

LOG_MODULE_REGISTER(modem_at_host, LOG_LEVEL_INF);

/* Stack definition for AT host workqueue */
K_THREAD_STACK_DEFINE(hp_at_host_stack_area, MODEM_AT_HOST_STACK_SIZE);

/* Maximum AT command message size */
#define AT_BUF_SIZE MODEM_AT_HOST_CMD_MAX_LEN

/*
The interface will use uart0 by default. However, modem,at-host-uart choice in devicetree can
be used to change the UART device
*/
#define AT_HOST_UART_DEV_GET() \
	DEVICE_DT_GET(COND_CODE_1(DT_HAS_CHOSEN(modem_at_host_uart), \
				  (DT_CHOSEN(modem_at_host_uart)), (DT_NODELABEL(uart0))))

#define IS_LOG_BACKEND_UART(_uart_dev) \
	(IS_ENABLED(CONFIG_LOG_BACKEND_UART) && \
	 COND_CODE_1( \
		 DT_HAS_CHOSEN(zephyr_log_uart), \
		 (_uart_dev == DEVICE_DT_GET(DT_CHOSEN(zephyr_log_uart))), \
		 (COND_CODE_1(DT_HAS_CHOSEN(zephyr_console), \
			      (_uart_dev == DEVICE_DT_GET(DT_CHOSEN(zephyr_console))), (false)))))

static const struct device *uart_dev = AT_HOST_UART_DEV_GET();
static bool at_buf_busy;         /* Guards at_buf while processing a command */
static char at_buf[AT_BUF_SIZE]; /* AT command and modem response buffer */
static struct k_work_q at_host_work_q;
static struct k_work cmd_send_work;

static inline void write_uart_string(const char *const str)
{
	if (IS_LOG_BACKEND_UART(uart_dev)) {
		/* The chosen AT host UART device is also the UART log backend device.
		 * Therefore, log the AT response instead of writing directly to the UART device.
		 */
		LOG_RAW("%s", str);
		return;
	}

	/* Send characters until, but not including, null */
	for (size_t i = 0; str[i]; i++) {
		uart_poll_out(uart_dev, str[i]);
	}
}

static void cmd_send(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);
	char response_buf[600];

	err = hyperpulse_lib_send_at_command(at_buf, response_buf, AT_BUF_SIZE);
	if (err < 0) {
		LOG_ERR("Error while processing AT command: %d", err);
	}

	write_uart_string(response_buf);

	at_buf_busy = false;
	uart_irq_rx_enable(uart_dev);
}

static void uart_rx_handler(uint8_t character)
{
	static bool inside_quotes;
	static size_t at_cmd_len;

	/* Handle control characters */
	switch (character) {
	/* Backspace and DEL character */
	case 0x08:
	case 0x7F:
		if (at_cmd_len > 0) {
			at_cmd_len--;
		}
		return;
	}

	/* Handle termination characters, if outside quotes. */
	if ((!inside_quotes) && (character == '\r')) {
		goto send;
	}

	/* Detect AT command buffer overflow, leaving space for null */
	if (at_cmd_len + 1 > sizeof(at_buf) - 1) {
		LOG_ERR("Buffer overflow, dropping '%c'\n", character);
		return;
	}

	/* Write character to AT buffer */
	at_buf[at_cmd_len] = character;
	at_cmd_len++;

	/* Handle special written character */
	if (character == '"') {
		inside_quotes = !inside_quotes;
	}

	return;
send:
	/* Terminate the command string */
	at_buf[at_cmd_len] = '\0';

	/* Reset UART handler state */
	inside_quotes = false;
	at_cmd_len = 0;

	/* Check for the presence of one printable non-whitespace character */
	for (const char *c = at_buf;; c++) {
		if (*c > ' ') {
			break;
		} else if (*c == '\0') {
			/* Drop command, if it has no such character */
			return;
		}
	}

	/* Send the command, if there is one to send */
	if (at_buf[0]) {
		/* Stop UART to protect at_buf */
		uart_irq_rx_disable(uart_dev);
		at_buf_busy = true;
		k_work_submit_to_queue(&at_host_work_q, &cmd_send_work);
	}
}

static void isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uint8_t character;

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	/*
	 * Check that we are not sending data (buffer must be preserved then),
	 * and that a new character is available before handling each character
	 */
	while ((!at_buf_busy) && (uart_fifo_read(dev, &character, 1))) {
		uart_rx_handler(character);
	}
}

static int at_uart_init(const struct device *uart_dev)
{
	int err;
	uint8_t dummy;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	uint32_t start_time = k_uptime_get_32();

	/* Wait for the UART line to become valid */
	do {
		err = uart_err_check(uart_dev);
		if (err) {
			if (k_uptime_get_32() - start_time > MODEM_AT_HOST_UART_INIT_TIMEOUT_MS) {
				LOG_ERR("UART check failed: %d. "
					"UART initialization timed out.",
					err);
				return -EIO;
			}

			LOG_INF("UART check failed: %d. "
				"Dropping buffer and retrying.",
				err);

			while (uart_fifo_read(uart_dev, &dummy, 1)) {
				/* Do nothing with the data */
			}
			k_sleep(K_MSEC(10));
		}
	} while (err);

	uart_irq_callback_set(uart_dev, isr);
	return err;
}

int modem_at_host_init(void)
{
	/* Initialize the UART module */
	int err = at_uart_init(uart_dev);
	if (err) {
		LOG_ERR("UART could not be initialized: %d", err);
		return -EFAULT;
	}

	k_work_init(&cmd_send_work, cmd_send);
	k_work_queue_start(&at_host_work_q, hp_at_host_stack_area,
			   K_THREAD_STACK_SIZEOF(hp_at_host_stack_area), MODEM_AT_HOST_THREAD_PRIO,
			   NULL);
	uart_irq_rx_enable(uart_dev);

	/* Signal ready */
	write_uart_string((char *)"Ready\r\n");

	return 0;
}

void hyperpulse_lib_unsolicited_at_response_app_handler(const char *const data)
{
	write_uart_string(data);
}
