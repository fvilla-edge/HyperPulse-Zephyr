/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  app_config.c
 * @brief application config file used by all samples
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
#include "hyperpulse_lib.h"

#if defined(CONFIG_HYPERPULSE_AUTO_LIB_INIT)
SYS_INIT(hyperpulse_lib_init, APPLICATION, 0);
#endif
