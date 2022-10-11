add_executable(stegogifigogets GifWatermarker.cpp)

target_include_directories(stegogifigogets PUBLIC
                            "${CMAKE_SOURCE_DIR}/include/")

target_link_libraries(stegogifigogets PUBLIC
                        "/usr/lib/x86_64-linux-gnu/libgif.a")
