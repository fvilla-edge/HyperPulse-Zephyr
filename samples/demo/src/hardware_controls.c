/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  hardware_controls.c
 * @brief Interface for hardware control.
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
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "hardware_controls.h"

#if IS_ENABLED(CONFIG_PM_DEVICE)
#include <zephyr/pm/device.h>
#endif

#if IS_ENABLED(CONFIG_GPIO)
#include <zephyr/drivers/gpio.h>
#endif

#if IS_ENABLED(CONFIG_REGULATOR)
#include <zephyr/drivers/regulator.h>
#endif

#if IS_ENABLED(CONFIG_SENSOR)
#include <zephyr/drivers/sensor.h>
#endif

#if IS_ENABLED(CONFIG_LED)
#include <zephyr/drivers/led.h>
#endif

#if IS_ENABLED(CONFIG_REGULATOR_NPM1300)
#include <zephyr/drivers/mfd/npm1300.h>

// Addresses
#define NPM1300_BUCK_BASE          0x04U
#define NPM1300_BUCK_OFFSET_EN_CLR 0x01U
#define NPM1300_BUCK_BUCKCTRL0     0x15U
#define NPM1300_BUCK_STATUS        0x34U

// Bits
#define NPM1300_BUCK2_MODE_BIT    BIT(0)
#define NPM1300_BUCK2_PULLDOWN_EN BIT(3)
#endif

LOG_MODULE_REGISTER(hw_ctrl, LOG_LEVEL_INF);

static int hardware_control_disable_accelerometer(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(accel0))
	const struct device *const sensor = DEVICE_DT_GET(DT_ALIAS(accel0));

	if (!device_is_ready(sensor)) {
		LOG_ERR("Could not get accel0 device");
		return -1;
	}

	// Disable the accelerometer
	struct sensor_value odr = {
		.val1 = 0,
	};

	int rc = sensor_attr_set(sensor, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
				 &odr);
	if (rc != 0) {
		LOG_ERR("Failed to set odr: %d", rc);
		return rc;
	}

	LOG_INF("Accelerometer disabled");
	return 0;

#else
	return -ENXIO;
#endif
}

static int hardware_control_disable_switch0(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

	gpio_pin_configure_dt(&sw0, GPIO_DISCONNECTED);

	LOG_INF("SW0 disabled");
	return 0;

#else
	return -ENXIO;
#endif
}

static int hardware_control_disable_external_flash(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(ext_flash))
	const struct device *const spi_nor = DEVICE_DT_GET(DT_ALIAS(ext_flash));

	// Disable external flash
	int err = pm_device_action_run(spi_nor, PM_DEVICE_ACTION_SUSPEND);
	if (err < 0) {
		LOG_ERR("Unable to suspend SPI NOR flash. (err: %d)", err);
		return err;
	}

	LOG_INF("External flash disabled");
	return 0;

#else
	return -ENXIO;
#endif
}

static int console_set_mode(bool enable)
{
#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_console))
	const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(console_dev)) {
		return -ENODEV;
	}

	int err = 0;
	enum pm_device_state state;

	err = pm_device_state_get(console_dev, &state);
	if (err != 0) {
		LOG_ERR("Failed to read console state (err: %d)", err);
		return err;
	}

	if (enable && (state != PM_DEVICE_STATE_ACTIVE)) {
		// Enable console UART
		err = pm_device_action_run(console_dev, PM_DEVICE_ACTION_RESUME);
		if (err < 0) {
			LOG_ERR("Unable to enable console (err: %d)", err);
			return err;
		}

	} else if (!enable && (state == PM_DEVICE_STATE_ACTIVE)) {
		// Disable console UART
		err = pm_device_action_run(console_dev, PM_DEVICE_ACTION_SUSPEND);
		if (err < 0) {
			LOG_ERR("Unable to suspend console (err: %d)", err);
			return err;
		}

		// Turn off to save power
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	} else {
		LOG_INF("Console is already %s", enable ? "enabled" : "disabled");
	}

	return 0;
#else
	return -ENXIO;
#endif
}

#if DT_NODE_EXISTS(DT_NODELABEL(npm1300_buck2)) && DT_NODE_EXISTS(DT_NODELABEL(npm1300_pmic))

#define USB_CONN_TASK_STACK_SIZE 500
#define USB_CONN_TASK_PRIORITY   5

K_THREAD_STACK_DEFINE(usb_conn_stack_area, USB_CONN_TASK_STACK_SIZE);
struct k_thread usb_conn_thread_data;

static volatile bool vbus_connected = false;

// USB detect semaphore
K_SEM_DEFINE(usb_detect_sem, 0, 1);

static int buck2_set_mode(bool enabled)
{
	const struct device *const buck2 = DEVICE_DT_GET(DT_NODELABEL(npm1300_buck2));
	const struct device *const pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_pmic));

	if (!device_is_ready(pmic)) {
		LOG_ERR("Failed to get PMIC device\n");
		return -ENODEV;
	}

	int err = 0;
	if (enabled) {
		err = regulator_enable(buck2);
		if (err < 0) {
			LOG_ERR("Failed to enable buck2: %d", err);
			return err;
		}

		err = mfd_npm1300_reg_update(pmic, NPM1300_BUCK_BASE, NPM1300_BUCK_BUCKCTRL0, 0,
					     NPM1300_BUCK2_PULLDOWN_EN);
		if (err < 0) {
			LOG_ERR("Failed to set buck2 pulldown. Err: %d", err);
			return err;
		}
	} else {
		err = regulator_disable(buck2);
		if (err < 0) {
			LOG_ERR("Failed to disable buck2: %d", err);
			return err;
		}
		err = mfd_npm1300_reg_update(pmic, NPM1300_BUCK_BASE, NPM1300_BUCK_BUCKCTRL0,
					     NPM1300_BUCK2_PULLDOWN_EN, NPM1300_BUCK2_PULLDOWN_EN);
		if (err < 0) {
			LOG_ERR("Failed to set buck2 pulldown. Err: %d", err);
			return err;
		}
	}

	return 0;
}

static void usb_connection_task(void *, void *, void *)
{
	for (;;) {
		// Wait for semaphore
		k_sem_take(&usb_detect_sem, K_FOREVER);

		// Check if USB is connected
		if (vbus_connected) {
			// Enable regulator
			buck2_set_mode(true);
			// Enable console
			console_set_mode(true);
			LOG_INF("USB is connected");
		} else {
			// Disable regulator
			buck2_set_mode(false);
			// Disable console
			console_set_mode(false);
		}
	}
}

static void usb_connection_callback(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED)) {
		vbus_connected = true;
		k_sem_give(&usb_detect_sem);
	}
	if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED)) {
		vbus_connected = false;
		k_sem_give(&usb_detect_sem);
	}
}

#endif

#define LED_ON_TIME_MS  100
#define LED_OFF_TIME_MS 1900

int hardware_control_flash_led(const led_id_t led_id, const int count)
{
#if DT_NODE_EXISTS(DT_NODELABEL(npm1300_led))
	static const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(npm1300_led));
#elif DT_NODE_EXISTS(DT_ALIAS(led0))
	static const struct device *led_dev = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(led0)));
#elif DT_NODE_EXISTS(DT_ALIAS(led1))
	static const struct device *led_dev = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(led1)));
#else
	return -ENXIO;
#endif

	if (!((led_id == LED_1 && DT_NODE_EXISTS(DT_ALIAS(led0))) ||
	      (led_id == LED_2 && DT_NODE_EXISTS(DT_ALIAS(led1))))) {
		return -ENXIO;
	}

	for (int i = 0; i < count; ++i) {
		led_on(led_dev, led_id);
		k_sleep(K_MSEC(LED_ON_TIME_MS));
		led_off(led_dev, led_id);

		if (i < count) {
			k_sleep(K_MSEC(LED_OFF_TIME_MS));
		}
	}

	return 0;
}

void hardware_control_init(void)
{
	// Enable console
	console_set_mode(true);

	// Power saving
	hardware_control_disable_accelerometer();
	hardware_control_disable_switch0();
	hardware_control_disable_external_flash();

	// Note: USB event callbacks have been setup for npm1300.
	// This will disable the console and buck2 when a USB connection is absent, and
	// re-enable when present.
#if DT_NODE_EXISTS(DT_NODELABEL(npm1300_buck2)) && DT_NODE_EXISTS(DT_NODELABEL(npm1300_pmic))

	// Setup callback for PMIC events
	static struct gpio_callback usb_cb;
	gpio_init_callback(&usb_cb, usb_connection_callback,
			   BIT(NPM1300_EVENT_VBUS_DETECTED) | BIT(NPM1300_EVENT_VBUS_REMOVED));

	static const struct device *const pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_pmic));
	mfd_npm1300_add_callback(pmic, &usb_cb);

	// Initialise vbus detection status
	static const struct device *const charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_charger));
	struct sensor_value val;

	int err = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);
	if (err == 0) {
		vbus_connected = (val.val1 != 0) || (val.val2 != 0);
		k_sem_give(&usb_detect_sem);
	} else {
		LOG_ERR("Failed to initialise vbus value");
	}

	// Create task to process USB connect/disconnect events
	k_thread_create(&usb_conn_thread_data, usb_conn_stack_area,
			K_THREAD_STACK_SIZEOF(usb_conn_stack_area), usb_connection_task, NULL, NULL,
			NULL, USB_CONN_TASK_PRIORITY, 0, K_NO_WAIT);
#endif

	return;
}
