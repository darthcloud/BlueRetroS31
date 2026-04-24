# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXCLUDE_COMPONENTS esp_encrypted_img)
project("BlueRetro$ENV{BR_HW}$ENV{BR_SYS}")
