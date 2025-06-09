function(compile_shaders SHADER_SOURCES)
    set(options)
    set(oneValueArgs OUTPUT_DIR GLSLC_FLAGS)
    set(multiValueArgs)
    cmake_parse_arguments(COMPILE_SHADERS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # fallback output dir
    if(COMPILE_SHADERS_OUTPUT_DIR)
        set(SHADER_OUTPUT_DIR "${COMPILE_SHADERS_OUTPUT_DIR}")
    else()
        set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    endif()

    file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

    foreach(SHADER_SOURCE ${SHADER_SOURCES})
        get_filename_component(FILE_NAME "${SHADER_SOURCE}" NAME_WE)
        get_filename_component(FILE_DIR "${SHADER_SOURCE}" DIRECTORY)
        file(RELATIVE_PATH FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${FILE_DIR}")
        get_filename_component(FILE_EXT "${SHADER_SOURCE}" EXT)
        string(REPLACE "." "" FILE_EXT "${FILE_EXT}")

        set(OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${FILE_DIR}/${FILE_NAME}-${FILE_EXT}.spv")
    
        add_custom_command(
            OUTPUT "${OUTPUT_FILE}"
            COMMAND glslc ${SHADER_SOURCE} -o "${OUTPUT_FILE}"
            DEPENDS "${SHADER_SOURCE}"
            COMMENT "Compiling shader ${SHADER_SOURCE} -> ${OUTPUT_FILE}"
            VERBATIM
        )

        # accumulate outputs in a global property so caller can define target
        list(APPEND COMPILED_SHADERS "${OUTPUT_FILE}")
    endforeach()

    # expose COMPILED_SHADERS back to caller scope
    set(COMPILED_SHADERS "${COMPILED_SHADERS}" PARENT_SCOPE)
endfunction()
