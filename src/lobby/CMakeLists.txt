cmake_minimum_required(VERSION 3.10)
cmake_policy( SET CMP0015 NEW )
project( Sapphire )

file( GLOB SERVER_PUBLIC_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*" )
file( GLOB SERVER_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}*.c*" )


add_executable( lobby ${SERVER_PUBLIC_INCLUDE_FILES} ${SERVER_SOURCE_FILES} )

target_link_libraries( lobby 
                         PRIVATE
                           common )
#cotire( lobby )
