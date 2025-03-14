# src/tools/csv_struct_gen/CMakeLists.txt
cmake_minimum_required( VERSION 3.0 )
cmake_policy(SET CMP0015 NEW)
project( csv_struct_gen )

file( GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h" )
file( GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" )

add_executable( csv_struct_gen
                ${HEADERS}
                ${SOURCES} )

target_link_libraries( csv_struct_gen
                       PRIVATE common )

# Add filesystem library for C++17 filesystem support
if( UNIX )
  target_link_libraries( csv_struct_gen
                         PRIVATE stdc++fs )
endif()

target_include_directories( csv_struct_gen
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                           PRIVATE "${CMAKE_SOURCE_DIR}/src/common"
                           PRIVATE "${CMAKE_SOURCE_DIR}/deps" )

# Set output directories to bin/tools instead of just bin
set_target_properties( csv_struct_gen
                      PROPERTIES 
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/tools"
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/tools"
                      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/tools"
                      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/tools" )
