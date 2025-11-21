/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  app_partition.c
 * @brief library partition info file used by all sample/test apps
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
#include <zephyr/storage/flash_map.h>
#include "hyperpulse_lib.h"

uint8_t hyperpulse_lib_app_hook_network_partition_id(void)
{
	return FIXED_PARTITION_ID(hyperpulse_storage_network);
}

uint8_t hyperpulse_lib_app_hook_platform_partition_id(void)
{
	return FIXED_PARTITION_ID(hyperpulse_storage_platform);
}
