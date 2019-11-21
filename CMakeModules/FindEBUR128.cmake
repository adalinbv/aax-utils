# Copyright (c) 2014 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# EBUR128_FOUND
# EBUR128_INCLUDE_DIR
# EBUR128_LIBRARY

set(ProgramFilesx86 "ProgramFiles(x86)")

find_path(EBUR128_INCLUDE_DIR
  NAMES ebur128.h
  HINTS
  "$ENV{${ProgramFilesx86}}/libebur128"
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt
)

find_library(EBUR128_LIBRARY
  NAMES ebur128
  HINTS
  "$ENV{${ProgramFilesx86}}/libebur128"
  PATH_SUFFIXES lib64
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /opt
)

if(EBUR128_LIBRARY AND EBUR128_INCLUDE_DIR)
  include_directories(${EBUR128INCLUDE_DIR})
  set(EBUR128FOUND "YES")
else()
  set(EBUR128FOUND "NO")
endif()
