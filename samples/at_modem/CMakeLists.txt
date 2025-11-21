# Copyright (c) 2024-2025, Myriota Pty Ltd, All Rights Reserved

cmake_minimum_required(VERSION 3.20.0)

list(APPEND OVERLAY_CONFIG
     $ENV{ZEPHYR_BASE}/../modules/lib/myriota-hyperpulse/lib/config/nrf91_soc.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(at_modem)

# Application sources
file(GLOB app_sources src/main.c src/modem_at_host.c src/modem.c)
target_sources(app PRIVATE ${app_sources})

include_directories(include)
