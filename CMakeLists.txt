# Only used by the experimental PlatformIO env `idf` (Arduino as an ESP-IDF component,
# see docs/framework-build.md). The default env `radio` ignores this file.
# The project name must not contain a space (the folder name does).
cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp_iradio)
