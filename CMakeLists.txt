# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

cmake_minimum_required(VERSION 3.24.2)
project("BluefishPlugins")

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
if (NOT WITH_NODOS)
	cmake_path(SET NODOS_SDK_DIR_PATH $ENV{NODOS_SDK_DIR})
	list(APPEND CMAKE_MODULE_PATH ${NODOS_SDK_DIR_PATH}/cmake)
    find_package(nosPluginSDK REQUIRED)
    set(TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Tools/${CMAKE_SYSTEM_NAME}")
    include(${CMAKE_CURRENT_SOURCE_DIR}/Scripts/CMake/GenerateFlatBuffers.cmake)
else()
    set(NODOS_BUILTIN_TYPES_DIR "${NOS_REPO_ROOT}/Source/Framework/nosFlatBuffers/FBS")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/Scripts/CMake/Plugins.cmake)

add_subdirectory(Bluefish444)
