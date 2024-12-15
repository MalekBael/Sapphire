cmake_policy(SET CMP0014 NEW)
cmake_minimum_required(VERSION 3.10)
project(Sapphire LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include custom CMake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
)

######################################
#           Boost Configuration      #
######################################
set(BOOST_ROOT "K:/local/boost_1_66_0")
set(BOOST_LIBRARYDIR "K:/local/boost_1_66_0/stage/lib")

# Find Boost libraries
find_package(Boost 1.66.0 REQUIRED COMPONENTS system thread)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found. Please check BOOST_ROOT and BOOST_LIBRARYDIR.")
endif()

# Optional: Display Boost details for verification
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# Create an interface library for Boost to simplify linking in subdirectories
add_library(BoostLib INTERFACE)
target_include_directories(BoostLib INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(BoostLib INTERFACE Boost::system Boost::thread)

#####################################
# Copy needed files to build-folder #
#####################################
add_custom_target(copy_runtime_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/config ${CMAKE_BINARY_DIR}/bin/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/sql ${CMAKE_BINARY_DIR}/bin/sql
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/web ${CMAKE_BINARY_DIR}/bin/web
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/sql_import.sh ${CMAKE_BINARY_DIR}/bin/sql_import.sh
)

# Copy data directory
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR}/bin)

######################################
# Dependencies and compiler settings #
######################################
include("cmake/paths.cmake")
include("cmake/compiler.cmake")

##############################
#             Git            #
##############################
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(VERSION --all --dirty=-d)

# Configure Version.cpp with Git information
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/common/Version.cpp.in"
               "${CMAKE_CURRENT_SOURCE_DIR}/src/common/Version.cpp" @ONLY)

##############################
#           Mysql            #
##############################
find_package(MySQL REQUIRED)

##############################
#        Dependencies        #
##############################
add_subdirectory("deps/zlib")
add_subdirectory("deps/MySQL")
add_subdirectory("deps/datReader")
add_subdirectory("deps/datReaderPs3")
add_subdirectory("deps/mysqlConnector")
add_subdirectory("deps/recastnavigation")

##############################
#  Main Sapphire Components  #
##############################
add_subdirectory("src/common")
add_subdirectory("src/api")
add_subdirectory("src/lobby")
add_subdirectory("src/world")
add_subdirectory("src/scripts")
add_subdirectory("src/dbm")

##############################
#           Tools            #
##############################
add_subdirectory("src/tools")
