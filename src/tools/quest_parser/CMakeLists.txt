cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0015 NEW)
project(Tool_QuestParser)

file(GLOB SERVER_PUBLIC_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*")
file(GLOB SERVER_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}*.c*")

add_executable(quest_parse ${SERVER_PUBLIC_INCLUDE_FILES} ${SERVER_SOURCE_FILES})

if (UNIX)
  target_link_libraries (quest_parse common xivdat pthread mysql dl z stdc++fs)
else()
  target_link_libraries (quest_parse common xivdat mysql zlib)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/unluac_2015_06_13.jar ${EXECUTABLE_OUTPUT_PATH}/unluac_2015_06_13.jar COPYONLY)