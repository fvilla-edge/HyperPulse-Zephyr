# Copyright (c) 2024-2025, Myriota Pty Ltd, All Rights Reserved

# This is a work around ncs_add_partition_manager_config() behavior Without this, we have to modify
# the partition manager subsystem to add a custom partition. See discussion here:
# https://devzone.nordicsemi.com/f/nordic-q-a/92581/ncs_add_partition_manager_config-not-working Add
# a partition manager configuration file to the build.
function(myriota_ncs_add_partition_manager_config config_file)
  get_filename_component(pm_path ${config_file} REALPATH)
  get_filename_component(pm_filename ${config_file} NAME)

  if(NOT EXISTS ${pm_path})
    message(FATAL_ERROR "Could not find specified partition manager configuration file "
                        "${config_file} at ${pm_path}")
  endif()

  set_property(GLOBAL APPEND PROPERTY PM_SUBSYS_PATHS ${pm_path})
  set_property(GLOBAL APPEND PROPERTY PM_SUBSYS_OUTPUT_PATHS
                                      ${CMAKE_CURRENT_BINARY_DIR}/${pm_filename})
endfunction()

zephyr_library()

zephyr_library_sources(src/app_config.c src/app_partition.c)

zephyr_library_import(libhyperpulse ${CMAKE_CURRENT_SOURCE_DIR}/archive/nrf9151_lib.a)

zephyr_include_directories(include)

myriota_ncs_add_partition_manager_config(pm.yml.hyperpulse)

# Store repo version
execute_process(
  COMMAND git describe --tags --always
  WORKING_DIRECTORY ${HYPERPULSE_MODULE_PATH} COMMAND_ECHO STDOUT ECHO_OUTPUT_VARIABLE
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE GIT_REPO_VERSION)

configure_file(${HYPERPULSE_MODULE_PATH}/git_version.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/include/git_version.h)

include_directories(include)

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE libhyperpulse)
