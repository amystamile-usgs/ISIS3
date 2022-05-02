# CMake module for find_package(OPENJPEG)
# Finds include directory and all applicable libraries
#
# Sets the following:
#   OPENJPEG_INCLUDE_DIR
#   OPENJPEG_LIBRARY

find_path(OPENJPEG_INCLUDE_DIR
    NAMES openjpeg.h
    PATH_SUFFIXES openjpeg-2.4
)

find_library(OPENJPEG_LIBRARY
  NAMES openjp2
)

message(STATUS "OPENJPEG INCLUDE DIR: "  ${OPENJPEG_INCLUDE_DIR} )
message(STATUS "OPENJPEG LIB: "  ${OPENJPEG_LIBRARY} )

get_filename_component(OPENJPEG_ROOT_INCLUDE_DIR "${OPENJPEG_INCLUDE_DIR}" DIRECTORY)
