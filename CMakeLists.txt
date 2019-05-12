cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(asio-uring LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(cmake/AsioUringFind.cmake)
find_package(Catch2)
find_package(Doxygen)
include(GNUInstallDirs)
if(Catch2_FOUND)
  find_package(Threads REQUIRED)
  enable_testing()
endif()
include(cmake/Functions.cmake)
include(cmake/Doxygen.cmake)
add_subdirectory(src)
install(EXPORT AsioUringTargets
        DESTINATION share/AsioUring/cmake
        NAMESPACE AsioUring::)
configure_file(cmake/AsioUringVariables.cmake.in "${CMAKE_BINARY_DIR}/AsioUringVariables.cmake" ESCAPE_QUOTES @ONLY)
install(FILES cmake/AsioUringFind.cmake
              cmake/AsioUringConfig.cmake
              "${CMAKE_BINARY_DIR}/AsioUringVariables.cmake"
              cmake/modules/FindUring.cmake
        DESTINATION share/AsioUring/cmake)
