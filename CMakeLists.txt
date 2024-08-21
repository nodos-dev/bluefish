# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

cmake_minimum_required(VERSION 3.24.2)
project("BluefishPlugins")

set(CMAKE_CXX_STANDARD 20)

set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)

# Dependencies
# ------------

# Nodos Plugin SDK
nos_find_sdk("1.2.0" NOS_PLUGIN_SDK_TARGET NOS_SUBSYSTEM_SDK_TARGET NOS_SDK_DIR)
nos_get_module("nos.sys.vulkan" "5.8" NOS_SYS_VULKAN_5_8_TARGET)

set(FLATC_EXECUTABLE ${NOS_SDK_DIR}/bin/flatc)

add_subdirectory(Bluefish444)