project(stc)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/serial.c
)

add_library(stc ${SOURCE_FILES})

target_include_directories(stc
        PUBLIC include/
)
