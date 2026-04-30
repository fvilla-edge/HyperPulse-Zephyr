#include <zephyr/kernel.h>

#include "downlink_num.h"

static K_MUTEX_DEFINE(downlink_num_mutex);
static bool downlink_num_available;
static uint8_t downlink_num_value;

void downlink_num_set(uint8_t value)
{
	k_mutex_lock(&downlink_num_mutex, K_FOREVER);
	downlink_num_value = value;
	downlink_num_available = true;
	k_mutex_unlock(&downlink_num_mutex);
}

bool downlink_num_get(uint8_t *value)
{
	if (value == NULL) {
		return false;
	}

	bool available;

	k_mutex_lock(&downlink_num_mutex, K_FOREVER);
	available = downlink_num_available;
	if (available) {
		*value = downlink_num_value;
	}
	k_mutex_unlock(&downlink_num_mutex);

	return available;
}
