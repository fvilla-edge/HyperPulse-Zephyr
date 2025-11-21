/****************************************************************
 * Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
 * @file  modem_at_host.h
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
#ifndef MODEM_AT_HOST_H
#define MODEM_AT_HOST_H

/**
 * @brief Initialize the AT host interface.
 *
 * Sets up the AT host interface, UART configuration, RX handling,
 * and prepares the interface to process AT commands and responses.
 *
 * @return  0        Initialization successful.
 * @return -EFAULT   UART setup failed.
 * @return -EINVAL   Invalid configuration detected.
 */
int modem_at_host_init(void);

#endif
