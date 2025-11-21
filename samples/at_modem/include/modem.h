/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  modem.h
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
#include <time.h>
#include <stdint.h>

#ifndef MODEM_H
#define MODEM_H

/**
 * @brief Maximum buffer size in bytes for storing an AT command response
 */
#define MODEM_AT_CMD_RESPONSE_BUF_SIZE 550

/**
 * @brief Enable downlink message notifications.
 * Downlink messages are received as Unsolicited Response Code (URC)
 * and can be retrieved through \p hyperpulse_lib_unsolicited_at_response_app_handler
 * @return 0 on success, negative on failure
 */
int32_t modem_enable_downlink_message_notification(void);

#endif
