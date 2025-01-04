cmake_minimum_required(VERSION 3.7.0)
set(cmake_verbose_makefile TRUE)

if (COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
endif (COMMAND CMAKE_POLICY)

include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFile)
include (CheckIncludeFiles)

PROJECT(aax-utils C CXX)
set(PACKAGE_NAME "AeonWave-utils")
set(PACKAGE "aaxutils")

option(WERROR "Treat compile warnings as errors"   OFF)
option(RMALLOC "Enable memory debugging functions" OFF)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(SIZEOF_SIZE_T 8)
else ()
  set(SIZEOF_SIZE_T 4)
endif ()

# read 'version' file into a variable (stripping any newlines or spaces)
FILE(READ aax_utils_version versionFile)
STRING(STRIP ${versionFile} AAXUTILS_VERSION)
set(VERSION AAXUTILS_VERSION)

# split version string into components, note CMAKE_MATCH_0 is the entire regexp match
STRING(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" CPACK_PACKAGE_VERSION ${AAXUTILS_VERSION})

set(AAX_UTILS_MAJOR_VERSION ${CMAKE_MATCH_1})
set(AAX_UTILS_MINOR_VERSION ${CMAKE_MATCH_2})
set(AAX_UTILS_MICRO_VERSION ${CMAKE_MATCH_3})
set(AAX_UTILS_PATCH_LEVEL ${DATE})

set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Utilities to show the capabilities of AeonWave")

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${aax-utils_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

include(CPack_Modules)
include(CPack_Defaults)

if (WIN32)

else(WIN32)
  ADD_DEFINITIONS(-D_REENTRANT -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=199309L)

  # DEBIAN
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.1), libaax4-bin (>= 4.0.0), libaax4-data (>= 4.0.0)")

  # RPM
  set(CPACK_RPM_PACKAGE_LICENSE "BSD")
  set(CPACK_RPM_PACKAGE_GROUP "System/System/GUI/KDE")
  set(CPACK_RPM_PACKAGE_REQUIRES "libaax4-bin >= 4.0.0, libaax4-data >= 4.0.0")
endif (WIN32)

# ZIP
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${AAXUTILS_VERSION}" CACHE INTERNAL "tarball basename")

# Add definitions, compiler switches, etc.
ADD_DEFINITIONS(-DHAVE_CONFIG_H=1)
if (WERROR)
  ADD_DEFINITIONS(-Werror)
endif ()

if (MSVC)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -D_DEBUG /LD")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -DNDEBUG /LD")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG /LD")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG /LD")
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE)

    OPTION(FORCE_STATIC_VCRT "Force /MT for static VC runtimes" ON)
    if (FORCE_STATIC_VCRT)
        FOREACH(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if (${flag_var} MATCHES "/MD")
                STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif ()
        ENDFOREACH(flag_var)
    endif ()
else(MSVC)
    if (MINGW)
       ADD_DEFINITIONS(-D__STRICT_ANSI__)
       set(CMAKE_SHARED_LINKER_FLAGS "${LD_FLAGS} -static-libstdc++" CACHE STRING
          "Flags used by MingW."
          FORCE)
    endif (MINGW)
 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" CACHE STRING
          "Flags used by al build types."
          FORCE)
    set(C_FLAGS "${C_FLAGS} -Os -Werror=format-security -std=c99 -Wall" CACHE STRING
        "Flags used by al build types."
        FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${C_FLAGS} -g -O2 -D_DEBUG" CACHE STRING
        "Flags used by the compiler during Release with Debug Info builds."
        FORCE)
    set(CMAKE_C_FLAGS_MINSIZEREL "${C_FLAGS} -Os -fomit-frame-pointer -DNDEBUG" CACHE STRING
        "Flags used by the compiler during release minsize builds."
        FORCE)
    set(CMAKE_C_FLAGS_RELEASE "${C_FLAGS} -Os -fomit-frame-pointer -DNDEBUG"
        CACHE STRING "Flags used by the compiler during release builds"
        FORCE)
    set(CMAKE_C_FLAGS_DEBUG "${C_FLAGS} -g3 -D_DEBUG" CACHE STRING
        "Flags used by the compiler during debug builds."
        FORCE)
endif (MSVC)

# Required libraries
find_package(AAX COMPONENTS aax REQUIRED)
find_package(XML COMPONENTS xml REQUIRED)

##find_package(AeonWave COMPONENTS aax REQUIRED)
if((GCC OR CLANG) AND RMALLOC)
  set(C_FLAGS "${C_FLAGS} -fsanitize=address")
  set(CXX_FLAGS "${CXX_FLAGS} -fsanitize=address")
  set(LDFLAGS "${LD_FLAGS} -fsanitize=address")
  set(RMALLOC OFF)
  find_package(XML REQUIRED)
else((GCC OR CLANG) AND RMALLOC)
  find_package(XML REQUIRED)
endif((GCC OR CLANG) AND RMALLOC)

# Add definitions, compiler switches, etc.
include_directories("${aax-utils_SOURCE_DIR}"
    include
    ${AAX_INCLUDE_DIR}
    "${aax-utils_BINARY_DIR}/include"
)

check_function_exists(strlcpy HAVE_STRLCPY)
check_include_FILE(inttypes.h HAVE_INTTYPES_H)
check_include_FILE(stdint.h HAVE_STDINT_H)
check_include_FILE(strings.h HAVE_STRINGS_H)
check_include_FILE(limits.h HAVE_LIMITS_H)
check_include_FILE(unistd.h HAVE_UNISTD_H)
check_include_FILE(sys/types.h HAVE_SYS_TYPES_H)
check_include_FILE(sys/time.h HAVE_SYS_TIME_H)
check_include_FILE(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_FILE(time.h HAVE_TIME_H)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/cmake_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.h")

# Man pages
if (UNIX AND NOT WIN32)
  CONFIGURE_FILE(
      "${CMAKE_CURRENT_SOURCE_DIR}/admin/aaxcvt.1.in"
      "${CMAKE_CURRENT_BINARY_DIR}/aaxcvt.1")
  CONFIGURE_FILE(
      "${CMAKE_CURRENT_SOURCE_DIR}/admin/aaxplay.1.in"
      "${CMAKE_CURRENT_BINARY_DIR}/aaxplay.1")
  CONFIGURE_FILE(
      "${CMAKE_CURRENT_SOURCE_DIR}/admin/aaxinfo.1.in"
      "${CMAKE_CURRENT_BINARY_DIR}/aaxinfo.1")

  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/aaxcvt.1
                ${CMAKE_CURRENT_BINARY_DIR}/aaxplay.1
                ${CMAKE_CURRENT_BINARY_DIR}/aaxinfo.1
          DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man1"
          COMPONENT Applications)
endif()

if (WIN32)
  set(KERNEL_LIBRARY Kernel32)
  set(EXTRA_LIBS "-ldriver -lwinmm")
  include_directories(base)
else(WIN32)
  set(EXTRA_LIBS "-ldriver -lm")
endif (WIN32)

include_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/src
)
link_directories(
  ${PROJECT_BINARY_DIR}/base
  ${PROJECT_BINARY_DIR}/src
)

add_subdirectory(base)
add_subdirectory(src)
add_subdirectory(3rdparty)
add_subdirectory(test)
add_subdirectory(test++)

if (NOT SINGLE_PACKAGE)
  include (CPack)
endif (NOT SINGLE_PACKAGE)

message("Build type: ${CMAKE_BUILD_TYPE}")
message("C flags: ${CMAKE_C_FLAGS}")
message("C++ flags: ${CMAKE_CXX_FLAGS}")
message("Linker flags: ${CMAKE_SHARED_LINKER_FLAGS}")

#-----------------------------------------------------------------------------
### uninstall target
#-----------------------------------------------------------------------------
#CONFIGURE_FILE(
#  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
#  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
#  IMMEDIATE @ONLY)
#ADD_CUSTOM_TARGET(uninstall
#  "${CMAKE_COMMAND}" -P "${aax-utils_BINARY_DIR}/cmake_uninstall.cmake")
