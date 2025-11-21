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
 * @brief Maximum downlink message size in bytes
 */
#define MODEM_DOWNLINK_MSG_SIZE 250

/**
 * @brief Maximum downlink message length in hexadecimal
 * Note: One byte is represented by two hexadecimal characters
 */
#define MODEM_DOWNLINK_MSG_HEX_SIZE (2 * MODEM_DOWNLINK_MSG_SIZE)

/**
 * @brief Maximum uplink message size in bytes
 */
#define MODEM_UPLINK_MSG_SIZE 250

/**
 * @brief Maximum uplink message length in hexadecimal
 * Note: One byte is represented by two hexadecimal characters
 */
#define MODEM_UPLINK_MSG_HEX_SIZE (2 * MODEM_UPLINK_MSG_SIZE)

/**
 * @brief Maximum buffer size in bytes for storing an AT command
 */
#define MODEM_AT_CMD_BUF_SIZE 550

/**
 * @brief Maximum buffer size in bytes for storing an AT command response
 */
#define MODEM_AT_CMD_RESPONSE_BUF_SIZE 550

/**
 * @brief Function pointer type for handler to process URC Modem Responses.
 * @param response Pointer to the URC response string
 */
typedef void (*process_urc_response_handler)(const char *const response);

/**
 * @brief Location methods for obtaining position
 */
typedef enum {
	MODEM_LOCATION_METHOD_GNSS /**< Use GNSS to obtain the location. */
} modem_location_method_t;

/**
 * @brief Location accuracy levels
 */
typedef enum {
	MODEM_LOCATION_ACCURACY_LOW,      /**< Low accuracy to save power */
	MODEM_LOCATION_ACCURACY_BALANCED, /**< Balanced power and accuracy */
	MODEM_LOCATION_ACCURACY_HIGH      /**< High accuracy, higher power usage */
} modem_location_accuracy_t;

/**
 * @brief Location request parameters
 */
typedef struct {
	modem_location_method_t method;     /**< Location method to use */
	modem_location_accuracy_t accuracy; /**< Accuracy level for the fix */
	uint32_t timeout_s;                 /**< Timeout in seconds */
	uint32_t max_age_last_update_s;     /**< Maximum age in seconds for last update of specified
					       method and accuracy to be considered valid */
} modem_location_request_t;

/**
 * @brief Geo-location structure
 */
typedef struct {
	int32_t latitude;     /**< Latitude in 0.1 micro-degrees */
	int32_t longitude;    /**< Longitude in 0.1 micro-degrees */
	int32_t elevation_mm; /**< Elevation in millimeters */
} modem_geo_location_t;

/**
 * @brief Location info returned from the modem
 */
typedef struct {
	modem_location_accuracy_t accuracy; /**< GNSS fix accuracy */
	modem_geo_location_t position;      /**< Geo-location coordinates */
	time_t timestamp_s;                 /**< Unix Epoch time of location fix */
} modem_location_info_t;

/**
 * @brief Enable downlink message notifications.
 * Downlink messages are received as Unsolicited Response Code (URC)
 * and can be retrieved through \p hyperpulse_lib_unsolicited_at_response_app_handler
 * @return 0 on success, negative on failure
 */
int32_t modem_enable_downlink_message_notification(void);

/**
 * @brief Get the received downlink message.
 * @param[in] downlink_message_at_response Pointer to the data retrieved from the URC handler
 * @param[out] downlink_message_hex Downlink message in hexadecimal format
 * @param[out] downlink_message_len Length of the received downlink message in bytes
 * @return 0 on success, negative on failure
 */
int modem_get_downlink_message(const char *const downlink_message_at_response,
			       char *downlink_message_hex, uint32_t *downlink_message_len);

/**
 * @brief Schedule a message for the NTN network
 * @param[in] uplink_msg Pointer to the binary data to be sent
 * @param[in] msg_len Length of the data in bytes
 * @return 0 on success, negative on failure
 */
int32_t modem_schedule_uplink_message(const char *const uplink_msg, const size_t msg_len);

/**
 * @brief Parse the location info from a modem AT command response
 * @param[in] location_at_cmd_response Buffer containing the location AT command response
 * @param[out] location Parsed location information
 * @return 0 on success, negative on failure
 */
int32_t modem_parse_location_response(const char *const location_at_cmd_response,
				      modem_location_info_t *const location);

/**
 * @brief Retrieve the current temperature measured by the modem
 * @param[out] temperature_celsius Pointer to store the temperature in degrees Celsius
 * @return 0 on success
 * @return -EINVAL Invalid argument or parse failure
 * @return -EIO AT command failed or modem returned an unexpected response
 */
int32_t modem_get_temperature_celsius(int8_t *const temperature_celsius);

/**
 * @brief Retrieve the current battery voltage
 * @param[out] voltage_mv Pointer to store the measured battery voltage in millivolts
 * @return 0 on success
 * @return -EINVAL Invalid argument or parse failure
 * @return -EIO AT command failed or modem returned an unexpected response
 */
int32_t modem_get_battery_voltage_mv(uint16_t *const voltage_mv);

/**
 * @brief Retrieve the current system time in seconds
 * @param[out] system_time_s Pointer to store the current system time in seconds since epoch
 * @return 0 on success
 * @return -EINVAL Invalid argument or parse failure
 * @return -EIO AT command failed
 */
int32_t modem_get_system_time_s(time_t *const system_time_s);

/**
 * @brief Request a GNSS-based location fix from the modem
 * @param[in] request Location request parameters (method, accuracy, max age, timeout)
 * @return 0 on success
 * @return -EIO AT command failed
 */
int32_t modem_request_location(const modem_location_request_t request);

#endif
