# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS "components/ahrs" "components/mpu9250" "main/wifi" "main/pinio" "main/orientation")

set(COMPONENTS bootloader esptool_py main ahrs mpu9250 wifi pinio orientation)  # This reduces the number of redundant components included

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(app-template)
