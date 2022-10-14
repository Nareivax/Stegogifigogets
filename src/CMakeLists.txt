add_library(gifwater STATIC GifWatermarker.cpp)

target_include_directories(gifwater PRIVATE
                                    "${CMAKE_SOURCE_DIR}/include/stegogifigogets")

target_link_libraries(gifwater PRIVATE
                                "/usr/lib/x86_64-linux-gnu/libgif.a")