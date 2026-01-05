#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
set(DTC_OVERLAY_FILE "boards/esp32s3_devkitc.overlay")
set(CMAKE_CXX_STANDARD 20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app C CXX)

# C++ level (matches your original)
target_compile_features(app PUBLIC cxx_std_20)

# Make all headers under src/ visible project-wide
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Hand off to src/
add_subdirectory(src)
