/****************************************************************
 * Copyright (c) 2025-2026, Myriota Pty Ltd, All Rights Reserved
 * @file  uart_uplink.h
 * @brief UART-triggered uplink listener API
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
#ifndef UART_UPLINK_H
#define UART_UPLINK_H

/**
 * @brief Start the UART line listener thread for uplink triggering.
 */
void uart_uplink_start(void);

#endif /* UART_UPLINK_H */
