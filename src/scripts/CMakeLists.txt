file(GLOB SCRIPT_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

message("exec: ${EXECUTABLE_OUTPUT_PATH}")

set(SCRIPT_POSTBUILD_DIR "${EXECUTABLE_OUTPUT_PATH}/compiledscripts/")
set(SCRIPT_LIB_DIR "${EXECUTABLE_OUTPUT_PATH}/tmp/compiledscripts/" )
set(EXECUTABLE_OUTPUT_PATH "${SCRIPT_LIB_DIR}")
set(LIBRARY_OUTPUT_PATH "${SCRIPT_LIB_DIR}")
set(RUNTIME_OUTPUT_DIRECTORY "${SCRIPT_LIB_DIR}")

if( WIN32 )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${SCRIPT_LIB_DIR}" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${SCRIPT_LIB_DIR}" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${SCRIPT_LIB_DIR}" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${SCRIPT_LIB_DIR}" )
endif()

# Add this debug info
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "EXECUTABLE_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH}")

# Jsons with gambit loaded here
file(GLOB_RECURSE JSON_FILES "${CMAKE_SOURCE_DIR}/data/**/**/*.json")
list(LENGTH JSON_FILES JSON_COUNT)
message(STATUS "Found ${JSON_COUNT} JSON files")

# Copy each JSON file while preserving directory structure
foreach(JSON_FILE ${JSON_FILES})
    # Get the relative path from source directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}" "${JSON_FILE}")
    # Set the destination path
    set(DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/${REL_PATH}")
    # Ensure destination directory exists
    get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${DEST_DIR}")
    # Copy the file
    configure_file("${JSON_FILE}" "${DEST_FILE}" COPYONLY)
    
    # Log which files we're copying (optional)
    message(STATUS "Copying JSON: ${REL_PATH}")
endforeach()

file(GLOB children "${CMAKE_CURRENT_SOURCE_DIR}/*" )
foreach(_scriptDir ${children})
    get_filename_component(_name "${_scriptDir}" NAME_WE)
    if(IS_DIRECTORY ${_scriptDir} AND NOT ${_name} MATCHES "CMakeFiles" AND NOT ${_name} MATCHES "\\.dir$")
        message("discovered plugin lib: ${_scriptDir} (${_name})")

        file(GLOB_RECURSE SCRIPT_BUILD_FILES "${_scriptDir}/*.cpp")
        file(GLOB_RECURSE SCRIPT_FILES RELATIVE "${_scriptDir}" "${_name}/*.cpp")

        foreach(_script ${SCRIPT_FILES})
            get_filename_component(_scriptname "${_script}" NAME_WE)

            if(NOT ${_scriptname} MATCHES "ScriptLoader")
                set(_fullpath "${_scriptDir}/${_script}")
                
                file(READ "${_fullpath}" _contents)
                set(_has_multiple_scripts FALSE)
                set(_exposed_classes "")
                file(STRINGS "${_fullpath}" _lines)
                
                foreach(_line ${_lines})
                    # Check for EXPOSE_SCRIPT and extract the class name
                    if(_line MATCHES "EXPOSE_SCRIPT[ ]*\\([ ]*([A-Za-z0-9_]+)[ ]*\\)")
                        string(REGEX MATCH "EXPOSE_SCRIPT[ ]*\\([ ]*([A-Za-z0-9_]+)[ ]*\\)" _match "${_line}")
                        string(REGEX REPLACE "EXPOSE_SCRIPT[ ]*\\([ ]*([A-Za-z0-9_]+)[ ]*\\)" "\\1" _exposed_class "${_match}")
                        
                        # Add to our list of exposed classes
                        if(_exposed_classes)
                            set(_exposed_classes "${_exposed_classes};${_exposed_class}")
                            set(_has_multiple_scripts TRUE)
                        else()
                            set(_exposed_classes "${_exposed_class}")
                        endif()
                    endif()
                endforeach()
                
                if(_exposed_classes)
                    string(REPLACE ";" " " _debug_classes "${_exposed_classes}")
                    message(STATUS "Found exposed scripts in ${_scriptname}.cpp: ${_debug_classes}")
                    
                    foreach(_class ${_exposed_classes})
                        if(ScriptIncludes)
                            set(ScriptIncludes "${ScriptIncludes}\nextern Sapphire::ScriptAPI::ScriptObject* makeScript${_class}();")
                        else()
                            set(ScriptIncludes "extern Sapphire::ScriptAPI::ScriptObject* makeScript${_class}();")
                        endif()
       
                        set(ScriptNames "${ScriptNames}   makeScript${_class}(),\n")
                    endforeach()
                else()
                    if(ScriptIncludes)
                        set(ScriptIncludes "${ScriptIncludes}\nextern Sapphire::ScriptAPI::ScriptObject* makeScript${_scriptname}();")
                    else()
                        set(ScriptIncludes "extern Sapphire::ScriptAPI::ScriptObject* makeScript${_scriptname}();")
                    endif()
                    
                    set(ScriptNames "${ScriptNames}   makeScript${_scriptname}(),\n")
                endif()
            endif()
        endforeach()

        add_library( "script_${_name}"
                       MODULE 
                         ${SCRIPT_BUILD_FILES} 
                         "${SCRIPT_INCLUDE_FILES}"
                         "${_scriptDir}/ScriptLoader.cpp" )

        target_link_libraries( "script_${_name}" world )

        target_include_directories("script_${_name}" PUBLIC "${CMAKE_SOURCE_DIR}/src/scripts")
        target_include_directories("script_${_name}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
        target_include_directories("script_${_name}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Scripts")

        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ScriptLoader.cpp.in" "${_scriptDir}/ScriptLoader.cpp")

        set(ScriptTargetName "script_${_name}")

        if(MSVC)
            add_custom_command(TARGET "script_${_name}" POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E remove "${SCRIPT_LIB_DIR}/script_${_name}.exp"
                COMMAND ${CMAKE_COMMAND} -E remove "${SCRIPT_LIB_DIR}/script_${_name}.lib"
                COMMAND ${CMAKE_COMMAND} -E remove "${SCRIPT_LIB_DIR}/script_${_name}.ilk"
                COMMAND ${CMAKE_COMMAND} -E copy "${SCRIPT_LIB_DIR}/script_${_name}.dll" ${SCRIPT_POSTBUILD_DIR}${ScriptTargetName}_LOCK
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SCRIPT_LIB_DIR}/script_${_name}.dll" ${SCRIPT_POSTBUILD_DIR}${ScriptTargetName}.dll
                COMMAND ${CMAKE_COMMAND} -E remove ${SCRIPT_POSTBUILD_DIR}${ScriptTargetName}_LOCK
            )
        else()
            add_custom_command(TARGET "script_${_name}" POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ScriptTargetName}> ${SCRIPT_POSTBUILD_DIR}${ScriptTargetName}_LOCK
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${ScriptTargetName}> ${SCRIPT_POSTBUILD_DIR}
                COMMAND ${CMAKE_COMMAND} -E remove ${SCRIPT_POSTBUILD_DIR}${ScriptTargetName}_LOCK
            )
        endif()

        unset(ScriptTargetName)
        unset(ScriptIncludes)
        unset(ScriptNames)
    endif()
endforeach()