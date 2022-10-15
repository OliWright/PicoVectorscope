
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/vector.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/points.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/idle.pio)

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
        ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/shapes.cpp
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
