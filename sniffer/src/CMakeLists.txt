project("sniffer")

add_executable(sniffer
        main.cpp
)

target_include_directories(sniffer
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external/kikan/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external/kikan/external/glfw/include
)

target_link_directories(${PROJECT_NAME} PUBLIC external/kikan/external/glfw/src)

target_link_directories(${PROJECT_NAME} PUBLIC external/kikan/src)
target_link_directories(${PROJECT_NAME} PUBLIC external/kikan/src/renderer)
target_link_directories(${PROJECT_NAME} PUBLIC external/kikan/src/ecs)

target_link_libraries(sniffer Kikan)