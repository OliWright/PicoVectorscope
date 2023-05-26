include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/bitmap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/bitmapfont.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/bitreversetable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/shapes3d.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/tilemap.cpp
)
