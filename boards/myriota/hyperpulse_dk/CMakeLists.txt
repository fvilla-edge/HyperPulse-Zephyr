# Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MYRIOTA_HYPERPULSE_DK_PMIC_STARTUP startup.c)
