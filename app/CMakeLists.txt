cmake_minimum_required(VERSION 3.4.1)
aux_source_directory(. src_files)

add_library( # Sets the name of the library.
        native-lib
        SHARED
        src/main/cpp/gif_main.cpp
        ${src_files}
        )
find_library(
        jnigraphics-lib
        jnigraphics)

target_link_libraries(native-lib
        log
        ${jnigraphics-lib}
        )