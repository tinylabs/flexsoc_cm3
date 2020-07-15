#
# Oracle cmake support - Compile simulator or synthesize FPGA gateware
# 
# All rights reserved.
# Tiny Labs Inc
# 2019
#
set( CMAKE_VERSION 3.2 )
cmake_minimum_required( VERSION ${CMAKE_VERSION} )
cmake_policy( VERSION ${CMAKE_VERSION} )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

# Create project
set( CMAKE_TOOLCHAIN_FILE ARM-Toolchain )
project( flexsoc_cm3 )

# Enable languages
enable_language( C CXX ASM )

# Include custom functions
include( Custom )

if( NOT CMAKE_CROSSCOMPILING )

  # Make sure verilator is installed
  find_package( verilator REQUIRED )

  # Make sure fusesoc is installed
  find_package( fusesoc REQUIRED )

  # Include common defs
  include_directories( common host/flexsoc )

  # Host application
  add_subdirectory( host )

  # Add tests
  add_subdirectory( test )
endif()

# Add tests
#add_subdirectory( test )
