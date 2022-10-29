include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/tilemap.cpp
)
