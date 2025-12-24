cmake_minimum_required(VERSION 3.15)
project(livePlaneDeformer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Maya SDK path - set this to your Maya installation
# Example: set(MAYA_LOCATION "C:/Program Files/Autodesk/Maya2018")
if(NOT DEFINED MAYA_LOCATION)
    if(DEFINED ENV{MAYA_LOCATION})
        set(MAYA_LOCATION $ENV{MAYA_LOCATION})
    else()
        message(FATAL_ERROR "MAYA_LOCATION not set. Please set MAYA_LOCATION environment variable or pass -DMAYA_LOCATION=<path>")
    endif()
endif()

message(STATUS "Maya location: ${MAYA_LOCATION}")

# Maya version detection
if(MAYA_LOCATION MATCHES "Maya([0-9]+)")
    set(MAYA_VERSION ${CMAKE_MATCH_1})
    message(STATUS "Detected Maya version: ${MAYA_VERSION}")
else()
    message(WARNING "Could not detect Maya version from path. Assuming Maya 2018.")
    set(MAYA_VERSION "2018")
endif()

# Platform-specific settings
if(WIN32)
    set(MAYA_PLUGIN_EXTENSION ".mll")
    set(MAYA_INCLUDE_DIR "${MAYA_LOCATION}/include")
    set(MAYA_LIBRARY_DIR "${MAYA_LOCATION}/lib")

    add_definitions(-DNT_PLUGIN -DREQUIRE_IOSTREAM -D_BOOL)

elseif(APPLE)
    set(MAYA_PLUGIN_EXTENSION ".bundle")
    set(MAYA_INCLUDE_DIR "${MAYA_LOCATION}/include")
    set(MAYA_LIBRARY_DIR "${MAYA_LOCATION}/Maya.app/Contents/MacOS")

    add_definitions(-DOSMac_ -DOSMac_MachO_ -DREQUIRE_IOSTREAM -D_BOOL)

else() # Linux
    set(MAYA_PLUGIN_EXTENSION ".so")
    set(MAYA_INCLUDE_DIR "${MAYA_LOCATION}/include")
    set(MAYA_LIBRARY_DIR "${MAYA_LOCATION}/lib")

    add_definitions(-DLINUX -DREQUIRE_IOSTREAM -D_BOOL)
endif()

# Maya libraries
find_library(MAYA_Foundation_LIBRARY Foundation PATHS ${MAYA_LIBRARY_DIR} NO_DEFAULT_PATH)
find_library(MAYA_OpenMaya_LIBRARY OpenMaya PATHS ${MAYA_LIBRARY_DIR} NO_DEFAULT_PATH)
find_library(MAYA_OpenMayaAnim_LIBRARY OpenMayaAnim PATHS ${MAYA_LIBRARY_DIR} NO_DEFAULT_PATH)

# Source files
set(SOURCE_FILES
    src/livePlaneDeformer.cpp
    src/livePlaneDeformer_lattice_functions.cpp
)

# Include directories
include_directories(
    ${MAYA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create the plugin
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# Link Maya libraries
target_link_libraries(${PROJECT_NAME}
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
)

# Set output properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ${MAYA_PLUGIN_EXTENSION}
    OUTPUT_NAME ${PROJECT_NAME}
)

# Windows-specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin"
    )
endif()

# macOS-specific settings
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-Wl,-exported_symbol,_initializePlugin -Wl,-exported_symbol,_uninitializePlugin"
    )
endif()

message(STATUS "Plugin will be built as: ${PROJECT_NAME}${MAYA_PLUGIN_EXTENSION}")
