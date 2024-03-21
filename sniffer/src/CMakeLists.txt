project("sniffer")

add_executable(sniffer
        main.cpp

        Sniffer.cpp
        Buffer.cpp

        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_impl_glfw.cpp
        imgui/imgui_impl_opengl3.cpp
        imgui/imgui_stdlib.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
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