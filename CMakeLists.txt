# Copyright (c) 2024-2025, Myriota Pty Ltd, All Rights Reserved

set(HYPERPULSE_MODULE_PATH
    $ENV{ZEPHYR_BASE}/../modules/lib/myriota-hyperpulse
    CACHE PATH "Path to HyperPulse Module")

add_subdirectory_ifdef(CONFIG_HYPERPULSE lib)
