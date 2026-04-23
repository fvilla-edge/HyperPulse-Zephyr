/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  configuration.c
 * @brief Configuration
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
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>

#include "configuration.h"
#include "periodic_uplink.h"

LOG_MODULE_REGISTER(configuration, LOG_LEVEL_INF);

#define CONFIG_SETT_KEY         "myriota"
#define CONFIG_SETT_MSG_PERIOD  "period"
#define CONFIG_SETT_GNSS_FIX    "gnss_fix"
#define CONFIG_SETT_HUM_ENABLE  "hum_enable"
#define DEFAULT_MSG_PERIOD_SECS 3600

#define STORAGE_PARTITION    storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION)

// The below variables are configurable through the shell interface
static uint32_t tx_msg_period = DEFAULT_MSG_PERIOD_SECS;
static bool enable_gnss_fix = false;
static bool enable_humidity = false;

// The mutexes are to ensure the configuration values are not
// read and written at the same time between the getters and setters.
K_MUTEX_DEFINE(msg_period_mutex);
K_MUTEX_DEFINE(gnss_fix_mutex);
K_MUTEX_DEFINE(hum_enable_mutex);

static int config_save_value(const char *sub_key, const void *data, size_t len)
{
	char key[32];
	snprintf(key, sizeof(key), "%s/%s", CONFIG_SETT_KEY, sub_key);

	int err = settings_save_one(key, data, len);
	if (err != 0) {
		LOG_ERR("Failed to save key %s (error: %d)", key, err);
		return err;
	}

	return 0;
}

uint32_t config_get_uplink_message_period(void)
{
	uint32_t period;

	k_mutex_lock(&msg_period_mutex, K_FOREVER);
	period = tx_msg_period;
	k_mutex_unlock(&msg_period_mutex);

	return period;
}

bool config_get_enable_gnss_state(void)
{
	bool enable;

	k_mutex_lock(&gnss_fix_mutex, K_FOREVER);
	enable = enable_gnss_fix;
	k_mutex_unlock(&gnss_fix_mutex);

	return enable;
}

bool config_get_enable_humidity_state(void)
{
	bool enable;

	k_mutex_lock(&hum_enable_mutex, K_FOREVER);
	enable = enable_humidity;
	k_mutex_unlock(&hum_enable_mutex);

	return enable;
}

static int config_set_tx_message_period(const uint32_t period)
{
	int err = config_save_value(CONFIG_SETT_MSG_PERIOD, &period, sizeof(period));
	if (err != 0) {
		return err;
	}

	k_mutex_lock(&msg_period_mutex, K_FOREVER);
	tx_msg_period = period;
	k_mutex_unlock(&msg_period_mutex);

	if (period > 0) {
		periodic_uplink_update_period(period);
	} else {
		periodic_uplink_stop();
	}

	return 0;
}

static int config_set_enable_gnss_state(bool enable)
{
	int err = config_save_value(CONFIG_SETT_GNSS_FIX, &enable, sizeof(enable));
	if (err != 0) {
		return err;
	}

	k_mutex_lock(&gnss_fix_mutex, K_FOREVER);
	enable_gnss_fix = enable;
	k_mutex_unlock(&gnss_fix_mutex);

	return 0;
}

static int config_set_enable_humidity_state(bool enable)
{
	int err = config_save_value(CONFIG_SETT_HUM_ENABLE, &enable, sizeof(enable));
	if (err != 0) {
		return err;
	}

	k_mutex_lock(&hum_enable_mutex, K_FOREVER);
	enable_humidity = enable;
	k_mutex_unlock(&hum_enable_mutex);

	return 0;
}

static int myriota_settings_set(const char *setting, size_t length, settings_read_cb read_cb,
				void *cb_arg)
{
	int err = 0;
	const char *next;

	if (settings_name_steq(setting, CONFIG_SETT_MSG_PERIOD, &next)) {
		err = read_cb(cb_arg, &tx_msg_period, sizeof(tx_msg_period));
		if (err >= 0) {
			return 0;
		}
		return err;

	} else if (settings_name_steq(setting, CONFIG_SETT_GNSS_FIX, &next)) {
		err = read_cb(cb_arg, &enable_gnss_fix, sizeof(enable_gnss_fix));
		if (err >= 0) {
			return 0;
		}
		return err;
	} else if (settings_name_steq(setting, CONFIG_SETT_HUM_ENABLE, &next)) {
		err = read_cb(cb_arg, &enable_humidity, sizeof(enable_humidity));
		if (err >= 0) {
			return 0;
		}
		return err;
	}

	return -ENOENT;
}

static struct settings_handler myriota_conf = {
	.name = "myriota",
	.h_set = myriota_settings_set,
};

int config_init(void)
{
	FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

	static struct fs_mount_t littlefs_mnt = {.type = FS_LITTLEFS,
						 .fs_data = &cstorage,
						 .storage_dev = (void *)STORAGE_PARTITION_ID,
						 .mnt_point = "/edge"};

	int err = fs_mount(&littlefs_mnt);
	if (err != 0) {
		LOG_ERR("Failed to mount littlefs (error: %d)", err);
	}

	err = settings_subsys_init();
	if (err != 0) {
		LOG_ERR("Settings subsystem init failed (err %d)", err);
		return err;
	}

	err = settings_register(&myriota_conf);
	if (err != 0) {
		LOG_ERR("Settings handler registration failed (err %d)", err);
		return err;
	}

	err = settings_load();
	if (err != 0) {
		LOG_ERR("Failed to load settings (err %d)", err);
		return err;
	}

	LOG_INF("Configuration initialized: period=%u s, GNSS=%s, Humidity=%s", tx_msg_period,
		enable_gnss_fix ? "enabled" : "disabled",
		enable_humidity ? "enabled" : "disabled");

	return 0;
}

static void sh_config_period(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_print(sh, "%u", config_get_uplink_message_period());
		return;
	}

	if (argc == 2) {
		uint32_t period = strtoul(argv[1], NULL, 10);
		if (config_set_tx_message_period(period) == 0) {
			shell_print(sh, "OK");
		}
		return;
	}

	shell_error(sh, "Usage: cfg period [seconds]");
}

static void sh_config_gnss_fix(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_print(sh, "%s", (config_get_enable_gnss_state() ? "enabled" : "disabled"));
		return;
	}

	if (argc == 2) {
		int val = strtol(argv[1], NULL, 10);
		if (val != true && val != false) {
			shell_error(sh, "Value must be 0 or 1");
			return;
		}

		if (config_set_enable_gnss_state(val) == 0) {
			shell_print(sh, "OK");
		}
		return;
	}

	shell_error(sh, "Usage: cfg gnss_fix [0|1]");
}

static void sh_config_hum_enable(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_print(sh, "%s", (config_get_enable_humidity_state() ? "enabled" : "disabled"));
		return;
	}

	if (argc == 2) {
		int val = strtol(argv[1], NULL, 10);
		if (val != true && val != false) {
			shell_error(sh, "Value must be 0 or 1");
			return;
		}

		if (config_set_enable_humidity_state(val) == 0) {
			shell_print(sh, "OK");
		}
		return;
	}

	shell_error(sh, "Usage: cfg hum_enable [0|1]");
}

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_cfg,
	SHELL_CMD(period, NULL,
		"Get/set message schedule period in seconds (default: 3600)",
		sh_config_period),
	SHELL_CMD(gnss_fix, NULL,
		"Get/set GNSS fix enable state (0=disable, 1=enable)",
		sh_config_gnss_fix),
	SHELL_CMD(hum_enable, NULL,
		"Get/set humidity field enable state (0=disable, 1=enable)",
		sh_config_hum_enable),
	SHELL_SUBCMD_SET_END
);
/* clang-format on */

SHELL_CMD_REGISTER(cfg, &sub_cfg, "Configuration commands", NULL);
