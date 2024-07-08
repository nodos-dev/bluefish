# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

cmake_minimum_required(VERSION 3.24.2)
project("BluefishPlugins")

set(CMAKE_CXX_STANDARD 20)

set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)

macro(get_targets targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_targets(${targets} ${subdir})
    endforeach()
    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(subtarget ${current_targets}) 
        if(TARGET ${subtarget})
            list(APPEND ${targets} ${subtarget})
        endif()
    endforeach()
endmacro()

macro(group_targets targets folder_name)
    foreach(target ${targets})
        get_target_property(FOLD ${target} FOLDER)
        if(${FOLD} STREQUAL "FOLD-NOTFOUND")
            set(FOLD_NAME "${folder_name}")
        else()
            set(FOLD_NAME "${folder_name}/${FOLD}")
        endif()
        set_target_properties(${target} PROPERTIES FOLDER ${FOLD_NAME})
    endforeach()
endmacro()

# Dependencies
# ------------

# Nodos Plugin SDK
nos_find_sdk("1.2.0" NOS_PLUGIN_SDK_TARGET NOS_SUBSYSTEM_SDK_TARGET NOS_SDK_DIR)
nos_get_module("nos.sys.vulkan" "5.2" NOS_SYS_VULKAN_5_2_TARGET)

set(FLATC_EXECUTABLE ${NOS_SDK_DIR}/bin/flatc)

add_subdirectory(Bluefish444)