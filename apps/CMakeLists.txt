add_executable(stegogifigogets Stegogifigogets.cpp)

target_include_directories(stegogifigogets PRIVATE
                                    "${CMAKE_SOURCE_DIR}/include/stegogifigogets")

target_link_libraries(stegogifigogets PRIVATE                                    
                                    gifwater)
