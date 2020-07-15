#
# Setup LeOS cross compile toolchain
# All rights reserved.
# Tiny Labs Inc
# 2018
#

if( BUILD_ARM )
  
  # Store LeOS version
  set( CMAKE_SYSTEM_NAME Generic )
  set( CMAKE_SYSTEM_PROCESSOR arm )
  
  # Force compiling static lib for compiler check as
  # linking will fail at this point
  set( CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY )
  
  # Save compilers
  find_program( CMAKE_C_COMPILER arm-none-eabi-gcc )
  if( NOT CMAKE_C_COMPILER )
    message( FATAL_ERROR "Please install arm-none-eabi-gcc - Required" )
  endif()
  find_program( CMAKE_CXX_COMPILER arm-none-eabi-g++ )
  if( NOT CMAKE_CXX_COMPILER )
    message( FATAL_ERROR "Please install arm-none-eabi-g++ - Required" )
  endif()

  # Set assembly flags
  #set( FLAGS "-march=armv7-m -mthumb --specs=nosys.specs" ) # default on gcc9
  set( FLAGS "-march=armv7-m -mthumb" )
  set( CMAKE_C_FLAGS "${FLAGS} ${CMAKE_C_FLAGS}" )
  set( CMAKE_ASM${ASM_DIALECT}_FLAGS "${FLAGS} ${CMAKE_ASM${ASM_DIALECT}_FLAGS}" )
  set( CMAKE_EXE_LINKER_FLAGS "-nostdlib ${CMAKE_EXE_LINKER_FLAGS}" )
  
endif( BUILD_ARM )

