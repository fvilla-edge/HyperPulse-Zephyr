# Copyright (c) 2025, Myriota Pty Ltd, All Rights Reserved

if(SB_CONFIG_HYPERPULSE_MERGE_APP_AND_NETWORK_INFO)

  set(SYSBUILD_APP_HEX ${APPLICATION_BINARY_DIR}/merged.hex)
  set(NETWORK_INFO_MERGED_HEX ${APPLICATION_BINARY_DIR}/app_and_network_info_merged.hex)
  set(NETWORK_INFO_TOOL ${CMAKE_CURRENT_SOURCE_DIR}/../tools/network_info.pyz)

  # Setup paths and names
  set(PARTITIONS_YAML_FILE ${APPLICATION_BINARY_DIR}/partitions.yml)
  set(TARGET_PARTITION_NAME "hyperpulse_storage_network")

  # Generate network info (Build-Time Dynamic Offset)
  add_custom_command(
    OUTPUT ${APPLICATION_BINARY_DIR}/network_info.hex
    COMMAND
      ${Python3_EXECUTABLE} ${NETWORK_INFO_TOOL} --partitions-file ${PARTITIONS_YAML_FILE}
      --partition-name "${TARGET_PARTITION_NAME}" -o ${APPLICATION_BINARY_DIR}/network_info.hex
    DEPENDS ${NETWORK_INFO_TOOL} ${SYSBUILD_APP_HEX}
    COMMENT "Generating network info hex using partitions file")

  # Merge network info with app
  add_custom_command(
    OUTPUT ${NETWORK_INFO_MERGED_HEX}
    COMMAND
      ${Python3_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py -o ${NETWORK_INFO_MERGED_HEX}
      ${SYSBUILD_APP_HEX} ${APPLICATION_BINARY_DIR}/network_info.hex
    DEPENDS ${SYSBUILD_APP_HEX} ${APPLICATION_BINARY_DIR}/network_info.hex
    COMMENT "Merging application HEX with network info HEX")

  add_custom_target(merge_network_info ALL DEPENDS ${NETWORK_INFO_MERGED_HEX})

endif()
