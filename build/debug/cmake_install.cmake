# Install script for directory: /mnt/data/005_git/crorc-driver-dma

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/opt/package/librorc/1.0.0")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Debug")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/mnt/data/005_git/crorc-driver-dma/include/mti.h"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/librorc" TYPE FILE FILES
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/device.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/patterngenerator.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/event_stream.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/event_generator.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/buffer.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/event_sanity_checker.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/link.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/sysmon.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/fastclusterfinder.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/sim_bar.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/eventfilter.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/dma_channel.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/bar.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/include_ext.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/ddl.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/diu.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/include_int.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/registers.h"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/siu.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/refclk.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/flash.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/bar_proto.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/defines.hh"
    "/mnt/data/005_git/crorc-driver-dma/include/librorc/microcontroller.hh"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FOREACH(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librorc.so.1.0.1"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librorc.so.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librorc.so"
      )
    IF(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      FILE(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    ENDIF()
  ENDFOREACH()
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/mnt/data/005_git/crorc-driver-dma/build/debug/librorc.so.1.0.1"
    "/mnt/data/005_git/crorc-driver-dma/build/debug/librorc.so.0"
    "/mnt/data/005_git/crorc-driver-dma/build/debug/librorc.so"
    )
  FOREACH(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librorc.so.1.0.1"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librorc.so.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librorc.so"
      )
    IF(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      FILE(RPATH_REMOVE
           FILE "${file}")
      IF(CMAKE_INSTALL_DO_STRIP)
        EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "${file}")
      ENDIF(CMAKE_INSTALL_DO_STRIP)
    ENDIF()
  ENDFOREACH()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("/mnt/data/005_git/crorc-driver-dma/build/debug/src/cmake_install.cmake")
  INCLUDE("/mnt/data/005_git/crorc-driver-dma/build/debug/fli/cmake_install.cmake")
  INCLUDE("/mnt/data/005_git/crorc-driver-dma/build/debug/tools_src/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)

FILE(WRITE "/mnt/data/005_git/crorc-driver-dma/build/debug/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "/mnt/data/005_git/crorc-driver-dma/build/debug/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
