# Copyright MediaZ AS. All Rights Reserved.

# Dependencies
# ------------
if (NOT WIN32)
    message(FATAL_ERROR "Unsupported platform: Currently, only Windows implementation is included")
endif()
add_subdirectory("${EXTERNAL_DIR}/BF444_WinSDK" "${CMAKE_CURRENT_BINARY_DIR}/External/BF444_WinSDK")

if (GENERATE_FLATBUFFERS_FILES)
    generate_flatbuffers("${TOOLS_DIR}/flatc" "${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "${CMAKE_CURRENT_SOURCE_DIR}/__temp__" "cpp")
endif()

get_targets(BLUEFISH_EXTERNAL_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/External/BF444_WinSDK")
group_targets("${BLUEFISH_EXTERNAL_TARGETS}" "External")

add_folder_as_plugin_project("Bluefish444" "bf444" "${CMAKE_CURRENT_SOURCE_DIR}/../../nos-modules/Subsystems/nosVulkanSubsystem/Include;")
