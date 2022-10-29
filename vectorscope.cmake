# CMAKE include file for the picovectorscope framework.
#
# Copyright (C) 2022 Oli Wright
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# A copy of the GNU General Public License can be found in the file
# LICENSE.txt in the root of this project.
# If not, see <https://www.gnu.org/licenses/>.
#
# oli.wright.github@gmail.com

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/idle.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/vector.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/points.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/raster.pio)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/buttons.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/dacout.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/dacoutputsm.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/fixedpoint.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/displaylist.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/gameshapes.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ledstatus.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/log.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/lookuptable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/serial.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/shapes.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sintable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/testcard.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/text.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/transform2d.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_dma
        hardware_pwm
        )
