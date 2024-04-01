# Copyright MediaZ AS. All Rights Reserved.

# Dependencies
# ------------
if (NOT WIN32)
    message(FATAL_ERROR "Unsupported platform: Currently, only Windows implementation is included")
endif()
add_subdirectory("${EXTERNAL_DIR}/BF444" "${CMAKE_CURRENT_BINARY_DIR}/External/BF444")

if (GENERATE_FLATBUFFERS_FILES)
    generate_flatbuffers("${TOOLS_DIR}/flatc" "${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "${CMAKE_CURRENT_SOURCE_DIR}/__temp__" "cpp" "${NODOS_BUILTIN_TYPES_DIR}")
endif()

get_targets(BLUEFISH_EXTERNAL_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/External/BF444")
group_targets("${BLUEFISH_EXTERNAL_TARGETS}" "External")

set(INCLUDE_FOLDERS "")
# For nodos in solution build
if(WITH_NODOS)
	list(APPEND INCLUDE_FOLDERS "${CACHED_NOS_SYS_VULKAN_INCLUDES}")
endif()

add_folder_as_plugin_project("Bluefish444" "bf444" "${INCLUDE_FOLDERS}")
