# ESP-IDF project — top-level CMakeLists.txt.
#
# This file tells the ESP-IDF build system the project name and pulls in the
# ESP-IDF build machinery. Nothing here is specific to a particular product:
# this repository is a template, and the project name is derived from the
# folder it sits in (see below).
#
# C++: the project is written in C++. ESP-IDF picks the newest standard the
# toolchain supports; with xtensa-esp-elf-g++ 15.2 that is -std=gnu++26
# (see $IDF_PATH/tools/cmake/build.cmake, preferred_cxx_versions).
#
# Two constraints follow from the default sdkconfig and shape the whole codebase:
#   CONFIG_COMPILER_CXX_EXCEPTIONS is not set  -> no throw/try/catch
#   CONFIG_COMPILER_CXX_RTTI       is not set  -> no dynamic_cast/typeid
# Errors are therefore returned as values (std::expected), never thrown.

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# ---------------------------------------------------------------------------
# Project name — taken from the repository folder name.
#
# This is what makes the repository a template: there is no product name baked
# into the source tree. Copy or clone the folder under a new name and the
# project name, the CMake targets and the output binary (build/<name>.elf)
# all follow automatically.
#
# CMake identifiers must match [A-Za-z0-9_], so anything else (spaces, dashes,
# dots, non-ASCII) is folded to an underscore, and a leading digit is prefixed.
# ---------------------------------------------------------------------------
get_filename_component(_project_dir_name "${CMAKE_CURRENT_LIST_DIR}" NAME)

string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _project_name "${_project_dir_name}")

if(_project_name MATCHES "^[0-9]")
    set(_project_name "p_${_project_name}")
endif()

message(STATUS "Project name: ${_project_name}  (from folder '${_project_dir_name}')")

project(${_project_name})
