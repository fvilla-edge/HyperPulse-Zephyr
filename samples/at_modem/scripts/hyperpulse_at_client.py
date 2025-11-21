#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved
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

import argparse
import os
import sys
import logging
import time
import re
import string
import threading
import serial.tools.list_ports
from datetime import datetime
from queue import Queue


VERSION = "0.1"


AT_CMD_MSEND = 'AT#MSEND={},"{}"\r'
AT_CMD_MRECV_ENABLE = "AT#MRECV=1\r"
AT_CMD_MRECV_DISABLE = "AT#MRECV=0\r"
AT_QUERY_MSWVERSION = "AT#MSWVERSION\r"
AT_RESPONSE_PATTERN_OK = r"(OK)"
AT_RESPONSE_PATTERN_MRECV = r"#MRECV: (\d+),\"([\da-fA-F]+)\""
AT_RESPONSE_PATTERN_MSWVERSION = r"Myriota HP lib v([\d\w\.-]+)"


class Timeout(int):
    def __new__(self, *args, **kwargs):
        return super(Timeout, self).__new__(self, *args, **kwargs)


TIMEOUT_NONE = Timeout()

DEFAULT_AT_TIMEOUT_S = 1


def arg_to_bool(s) -> bool:
    if isinstance(s, bool):
        return s
    elif isinstance(s, str):
        return s.lower() in ["true", "t", "yes", "y", "1"]
    else:
        raise TypeError(f"argument {s} has unknown type, bool expected")


class MyriotaHyperPulseAT:
    def __init__(self, port: str, baudrate: int):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.ser = None
        self.port = port
        self.baudrate = baudrate

        self.at_passthrough_mode = False

        self.is_active = False
        self.tx_lock = threading.Lock()
        self.tx_queue = Queue()
        self.rx_queue = Queue()

        self.send_thread = threading.Thread(target=self._sendloop, args=(), daemon=True)
        self.read_thread = threading.Thread(target=self._readloop, args=(), daemon=True)

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        self.close()

    def open(self):
        if self.is_active:
            raise RuntimeError("Myriota HyperPulse client already open")
        self.is_active = True

        self.logger.debug(f"Opening serial port {self.port} {self.baudrate}")
        self.ser = serial.Serial(port=self.port, baudrate=self.baudrate, timeout=0)
        self.send_thread.start()
        self.read_thread.start()

    def close(self, flush_tx_queue=True):
        if flush_tx_queue:
            while not self.tx_queue.empty() or self.tx_lock.locked():
                time.sleep(0.01)
        self.is_active = False
        self.send_thread.join()
        self.read_thread.join()
        if self.ser and self.ser.is_open:
            self.ser.close()

    def _at_query(self, query, timeout_s=TIMEOUT_NONE) -> str:
        self.tx_queue.put(query)
        return self.get_response(timeout_s)

    def _sendloop(self):
        while self.is_active:
            if not self.tx_queue.empty():
                try:
                    self.tx_lock.acquire()
                    next = self.tx_queue.get()
                    self.logger.debug(f"TX: {next}")

                    # ensure cmd ends with \r
                    if "\r" not in next:
                        next = f"{next}\r"

                    self.ser.write(next.encode("UTF-8"))
                except Exception as e:
                    self.logger.debug(f"Serial write failure: {e}")
            else:
                time.sleep(0.01)

    def _readloop(self):
        response = ""
        while self.is_active:
            try:
                response += self.ser.readline().decode("UTF-8")
            except UnicodeDecodeError as e:
                self.logger.debug(f"Serial decode failure: {e}: {response}")
            except Exception as e:
                self.logger.debug(f"Serial read failure: {e}")

            if "\r" not in response:
                continue

            response = response.strip()
            if not response:
                continue

            self.logger.debug(f"RX: {response.strip()}")

            queue_response = False
            if "ERROR" in response and self.tx_lock.locked():
                self.logger.debug("Command failed")
                self.tx_lock.release()
            elif "OK" == response and self.tx_lock.locked():
                self.logger.debug("Command completed")
                self.tx_lock.release()
            else:
                queue_response = True

            if queue_response or self.is_at_passthrough():
                self.rx_queue.put(response)

            response = ""

    def set_at_passthrough(self, enabled: bool) -> None:
        # wait for queued tx data to complete
        while not self.tx_queue.empty():
            time.sleep(0.01)
        # wait for any currently executing AT command to complete
        self.tx_lock.acquire()
        self.at_passthrough_mode = enabled
        self.tx_lock.release()

    def is_at_passthrough(self) -> bool:
        return self.at_passthrough_mode

    def get_response(self, timeout_s: Timeout = 0) -> str:
        response = ""

        if timeout_s is TIMEOUT_NONE:
            is_expired = lambda: False
        else:
            t_timeout = time.perf_counter() + timeout_s
            is_expired = lambda: time.perf_counter() > t_timeout

        while not is_expired():
            if not self.rx_queue.empty() and not self.tx_lock.locked():
                break
            time.sleep(0.01)

        if not self.rx_queue.empty():
            response += self.rx_queue.get()

        return response

    def query_version(self) -> str:
        self.logger.debug("Querying library version")
        ver_response = self._at_query(
            query=AT_QUERY_MSWVERSION,
            timeout_s=DEFAULT_AT_TIMEOUT_S,
        )
        re_match = re.findall(AT_RESPONSE_PATTERN_MSWVERSION, ver_response)
        if re_match:
            ver = re_match[0]
            self.logger.debug(f"Myriota HyperPulse library software version: {ver}")
            return ver
        else:
            raise ValueError(f"invalid data: {ver_response}")

    def send_message(self, message_string: str) -> None:
        if not all(char in string.hexdigits for char in message_string):
            raise ValueError("message contains non hex characters")
        if len(message_string) % 2 != 0:
            raise ValueError("message contains incomplete byte")

        len_bytes = int(len(message_string) / 2)
        self.logger.debug(f"Sending message: {message_string} ({len_bytes})")

        send_cmd = AT_CMD_MSEND.format(len_bytes, message_string)
        self.tx_queue.put(send_cmd)

    def enable_downlink_notifications(self, enabled: bool) -> None:
        if enabled:
            self.logger.debug("Enabling downlink notifications")
            self.tx_queue.put(AT_CMD_MRECV_ENABLE)
        else:
            self.logger.debug("Disabling downlink notifications")
            self.tx_queue.put(AT_CMD_MRECV_DISABLE)
        # wait for command to complete
        while self.tx_lock.locked():
            time.sleep(0.01)
        self.logger.debug(
            "Downlink notifications %s" % "enabled" if enabled else "disabled"
        )

    def send_at_cmd(self, at_cmd_passthrough: str) -> None:
        self.tx_queue.put(at_cmd_passthrough)


class MyriotaHyperPulseConsole:
    def __init__(self, myriota: MyriotaHyperPulseAT, logger: logging.Logger):
        self.myriota = myriota
        self.logger = logger

    def enable_at_passthrough(self, enable: bool) -> None:
        self.logger.debug("%s AT passthrough" % "Enabling" if enable else "Disabling")
        self.myriota.set_at_passthrough(enable)

    def run(self) -> None:
        # rx print thread
        recv_thread = threading.Thread(
            target=self._recv_stdout_loop, args=(), daemon=True
        )
        recv_thread.start()

        # tx/stdin on main thread
        self._send_stdin_loop()

    def _send_stdin_loop(self) -> None:
        while True:
            try:
                data = input().strip()
                if data == "@":
                    enter_exit = (
                        "Exiting" if self.myriota.is_at_passthrough() else "Entering"
                    )
                    self.logger.info(f"{enter_exit} AT passthrough mode")
                    self.myriota.set_at_passthrough(
                        not self.myriota.is_at_passthrough()
                    )
                    continue

                if data:
                    if self.myriota.is_at_passthrough():
                        self.logger.info(f"Passing AT command {data}")
                        self.myriota.send_at_cmd(data)
                    else:
                        self.logger.info(f"Sending message {data}")
                        self.myriota.send_message(data)
            except ValueError as e:
                self.logger.error(f"error sending message: {e}")
            except TimeoutError as e:
                self.logger.error(f"device did not acknowledge message")
            except EOFError as e:
                break
            except KeyboardInterrupt:
                self.logger.info("Quitting")
                break  # graceful Ctrl-C exit

    def _recv_stdout_loop(self) -> None:
        while True:
            try:
                recv_raw = self.myriota.get_response(timeout_s=0)
                if self.myriota.is_at_passthrough():
                    if recv_raw:
                        print(recv_raw)
                else:
                    # only display downlink notifications
                    re_match = re.findall(AT_RESPONSE_PATTERN_MRECV, recv_raw)
                    if re_match:
                        msg_len = int(re_match[0][0])
                        msg_str = re_match[0][1]
                        msg_str_bytes = len(msg_str) / 2
                        if msg_len == msg_str_bytes:
                            print(msg_str)
                        else:
                            print(
                                f"Received downlink message of length {msg_len} but only received {int(msg_str_bytes)} bytes: {msg_str}"
                            )
            except TimeoutError:
                pass
            finally:
                time.sleep(0.01)


def run_main(argv):
    parser = argparse.ArgumentParser(
        description="Myriota HyperPulse™ Message tool",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    all_ports = [p.device for p in serial.tools.list_ports.comports()]
    default_port = all_ports[-1] if all_ports else None

    current_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    default_log_filename = "hyperpulse_messages_" + current_time + ".log"

    parser.add_argument("-p", "--port", default=default_port, help="port name")
    parser.add_argument("-b", "--baudrate", default=115200, help="baud rate")
    parser.add_argument("-l", "--log", default=True, help="enable logging")
    parser.add_argument(
        "--log-filename", default=default_log_filename, help="log filename"
    )
    parser.add_argument(
        "-v", "--version", action="store_true", help="Prints script version"
    )
    parser.add_argument(
        "--query-version",
        action="store_true",
        help="Queries the Myriota HyperPulse™ library software version",
    )
    parser.add_argument(
        "-a",
        "--at-passthrough",
        default=False,
        action="store_true",
        help="Begin in AT passthrough mode. Enter @ to enter AT mode interactively.",
    )

    args = parser.parse_args(argv)

    if args.version:
        print(f"Myriota HyperPulse™ Message Scheduler tool v{VERSION}")
        return

    # send info messages to stdout, all logs to file
    enable_file_log = arg_to_bool(args.log)
    logger = logging.getLogger(__name__)
    if enable_file_log:
        logging.basicConfig(
            filename=args.log_filename,
            filemode="a",
            format="[%(asctime)s] %(levelname)s [%(name)s.%(funcName)s:%(lineno)d] %(message)s",
            level=logging.DEBUG,
        )
        logging.Formatter.default_msec_format = "%s.%03d"
        logging_handler = logging.StreamHandler(sys.stderr)
        logging_handler.setLevel(logging.INFO)
        logging_handler.setFormatter(
            logging.Formatter("[%(asctime)s %(levelname)s %(name)s] %(message)s")
        )
        logger.addHandler(logging_handler)
        logger.info(f"Logging to {args.log_filename}")
    else:
        logging.basicConfig(filename=None, format="%(message)s", level=logging.CRITICAL)

    with MyriotaHyperPulseAT(port=args.port, baudrate=args.baudrate) as hyper_pulse:
        if args.query_version:
            try:
                print(hyper_pulse.query_version())
            except TimeoutError as e:
                logger.error(f"Failed to query HyperPulse version: {e}")
        else:
            hyper_pulse.enable_downlink_notifications(True)

            myriota_console = MyriotaHyperPulseConsole(
                myriota=hyper_pulse, logger=logger
            )
            myriota_console.enable_at_passthrough(args.at_passthrough)
            myriota_console.run()

            # turn off dl notifications before exiting
            hyper_pulse.enable_downlink_notifications(False)


if __name__ == "__main__":
    run_main(sys.argv[1:])
