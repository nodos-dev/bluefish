# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

# Dependencies
# ------------
if (NOT WIN32)
    message(FATAL_ERROR "Unsupported platform: Currently, only Windows implementation is included")
endif()
add_subdirectory("${EXTERNAL_DIR}/BF444" "${CMAKE_CURRENT_BINARY_DIR}/External/BF444")

nos_generate_flatbuffers("${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "cpp" "${NOS_SDK_DIR}/types" Bluefish444_generated)

get_targets(BLUEFISH_EXTERNAL_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/External/BF444")
group_targets("${BLUEFISH_EXTERNAL_TARGETS}" "External")

set(INCLUDE_FOLDERS "")

nos_add_plugin("Bluefish444" "bf444;Bluefish444_generated;${NOS_SYS_VULKAN_5_2_TARGET};${NOS_PLUGIN_SDK_TARGET}" "${INCLUDE_FOLDERS}")
group_targets("Bluefish444;Bluefish444_generated" "Bluefish Plugins")
