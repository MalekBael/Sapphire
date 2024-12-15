cmake_policy(SET CMP0015 NEW)

# Gather source files recursively
file(GLOB SERVER_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.cpp
    *.h
    Actor/*.cpp
    Actor/*.h
    Action/*.cpp
    Action/Job/*.cpp
    AI/*.cpp
    AI/Fsm/*.cpp
    ContentFinder/*.cpp
    DebugCommand/*.cpp
    Event/*.cpp
    FreeCompany/*.cpp
    Inventory/*.cpp
    Linkshell/*.cpp
    Manager/*.cpp
    Math/*.cpp
    Network/*.cpp
    Network/Util/*.cpp
    Network/Handlers/*.cpp
    Network/PacketWrappers/*.cpp
    Script/*.cpp
    StatusEffect/*.cpp
    Territory/*.cpp
    Territory/Housing/*.cpp
    Util/*.cpp
    Navi/*.cpp
    Task/*.cpp
    Quest/*.cpp
)

# Define the executable
add_executable(world ${SERVER_SOURCE_FILES})

# Set target properties
set_target_properties(world
    PROPERTIES
        ENABLE_EXPORTS ON
        WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Link libraries, including Boost via the BoostLib interface library
target_link_libraries(world
    PUBLIC
        common 
        Detour
        DetourCrowd 
        BoostLib
)

# Include directories specific to the target
target_include_directories(world
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        Detour
)