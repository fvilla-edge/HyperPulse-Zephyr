#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2021-2024, Myriota Pty Ltd, All Rights Reserved
# SPDX-License-Identifier: BSD-3-Clause-Attribution
#
# This file is licensed under the BSD with attribution  (the "License"); you
# may not use these files except in compliance with the License.
#
# You may obtain a copy of the License here:
# LICENSE-BSD-3-Clause-Attribution.txt and at
# https://spdx.org/licenses/BSD-3-Clause-Attribution.html
#
# See the License for the specific language governing permissions and
# limitations under the License.


# Unpacker for the Demo Application
# Usage:
# unpack.py -x <message_data>
# or
# echo "<message_data>" | unpack.py

import argparse
import struct
import json
import fileinput

MESSAGE_FORMAT = "<IIiihbH"
MESSAGE_SIZE = struct.calcsize(MESSAGE_FORMAT)


def unpack(packet: str):
    data = bytearray.fromhex(packet)
    if len(data) < MESSAGE_SIZE:
        raise ValueError(
            f"Packet too short: got {len(data)} bytes, need {MESSAGE_SIZE}"
        )

    (
        sequence_number,
        time,
        latitude,
        longitude,
        elevation,
        temperature,
        battery_voltage,
    ) = struct.unpack(MESSAGE_FORMAT, data[:MESSAGE_SIZE])

    return {
        "Sequence Number": sequence_number,
        "Time (epoch s)": time,
        "Latitude": latitude * 1e-7,
        "Longitude": longitude * 1e-7,
        "Altitude (meters)": elevation,
        "Temperature (degrees C)": temperature,
        "Battery Voltage (mV)": battery_voltage,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Unpack hexadecimal data from Level Monitoring App",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "-x", "--hex", type=str, default="-", help="Packet data in hexadecimal format"
    )
    args = parser.parse_args()

    results = []
    if args.hex == "-":
        for line in fileinput.input():
            results.append(unpack(line.strip()))
    else:
        results.append(unpack(args.hex))

    print(json.dumps(results, indent=2))
