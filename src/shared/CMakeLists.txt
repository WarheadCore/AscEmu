# Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

# Group sources
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(shared
  ${PRIVATE_SOURCES})

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(shared
  PUBLIC
    # Provide the binary dir for all child targets
    ${CMAKE_BINARY_DIR}
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(shared
  PRIVATE
    ascemu-core-interface
  PUBLIC
    zlib
    mysql
    utf8cpp
    openssl
    threads
    stdfs
    ${EXTRA_LIBS})

# Generate precompiled header
if (USE_PCH)
  add_cxx_pch(shared PrecompiledHeaders/sharedPCH.h)
endif ()
