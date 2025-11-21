/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  hyperpulse_lib.h
 * @brief Myriota HyperPulse™ Library API
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
#ifndef HYPERPULSE_LIB_H
#define HYPERPULSE_LIB_H

/********************************************
 * Includes
 *********************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/********************************************
 * Library APIs
 *********************************************/

/**
 * @defgroup hyperpulse_lib_api_group HyperPulse™ library API
 * This is the group of APIs supported by Myriota HyperPulse™ library.
 * @{
 */

/**
 * @brief Initialise the Myriota HyperPulse™ library.
 * @return 0 on success, error code on failure.
 */
int32_t hyperpulse_lib_init(void);

/**
 * @brief Return status of Myriota HyperPulse™ library initialisation.
 * @return true on successful initialisation, false on failure or fault state.
 */
bool hyperpulse_lib_is_initialised(void);

/**
 * @brief Send AT command in ASCII format to the modem and
 * receive ASCII response in buffer of specified length.

 * @param cmd             AT command string buffer in ASCII format.
 * @param[out] response   Buffer to receive modem response into.
 * @param response_len    Byte length of provided response buffer.
 * @return 0 on success, error code on failure.
 */
int32_t hyperpulse_lib_send_at_command(
        const char *const cmd, char *const response, size_t response_len);

/**
 * @brief APP defined notification function that library uses to send
 * unsolicited modem response.

  * @param response  Unsolicited modem response in ASCII string format.
 */
void hyperpulse_lib_unsolicited_at_response_app_handler(const char *const response);

/** @} */ // end of hyperpulse_lib_api_group

#ifdef __cplusplus
}
#endif

#endif // HYPERPULSE_LIB_H
