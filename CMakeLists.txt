cmake_minimum_required(VERSION 3.16)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# set the project name
project(Stegogifigogets VERSION 0.2
                        DESCRIPTION "Gif Steganography Program"
                        LANGUAGES CXX)

if(${CMAKE_BUILD_TYPE} MATCHES debug)
    add_compile_definitions(DEBUG=true)
endif()

add_subdirectory(src)
add_subdirectory(apps)