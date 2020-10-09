#
# Custom cmake functions to support this project
#
# Set test directory
  
if( CMAKE_CROSSCOMPILING )

  set( TEST_ARM_DIR ${PROJECT_SOURCE_DIR}/test/arm )
  
  function( target_bin TARGET SOURCES )
    
    # Create executable
    add_executable( ${TARGET} ${SOURCES} ${ARGN} )
    
    # Link to common lib
    target_link_libraries( ${TARGET} arm c )

    # Set link properties
    set_target_properties( ${TARGET} PROPERTIES
      LINK_FLAGS
      "-Wl,-zmax-page-size=4 -T ${TEST_ARM_DIR}/linker.ld -Wl,-Map=${TARGET}.map"
      COMPILE_FLAGS
      "-O0 -ggdb" )
    
    # Create binary and copy back
    add_custom_command( TARGET ${TARGET} POST_BUILD
      DEPENDS ${TARGET}
      COMMENT "Creating binary: ${TARGET}.bin"
      COMMAND arm-none-eabi-objcopy -O binary ${TARGET} ${TARGET}.bin
      COMMAND cmake -E copy $<TARGET_FILE:${TARGET}>.bin ${TEST_ARM_DIR}/bin/
      )
    
    # Cleanup
    set_property( DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${TARGET}.bin )
    
  endfunction( target_bin )
  
else( NOT CROSSCOMPILING )
  
  # Create plugin
  function( plugin NAME SOURCES )
    
    # Create shared library
    add_library( ${NAME} SHARED ${SOURCES} ${ARGN} )
    
    # Make sure our export structure doesn't get compiled out
    set_target_properties( ${NAME} PROPERTIES
#      COMPILE_FLAGS "-O0 -ggdb"
      LINK_FLAGS "-Wl,-entry=__plugin" )
    
    # Link against plugin for helper fns
    target_link_libraries( ${NAME} pluginhelper )
    
  endfunction( plugin )

  macro( gw_target NAME )
    add_custom_target( ${NAME}
      COMMENT "Generating gateware for ${NAME}..."
      COMMAND ${FUSESOC_EXECUTABLE} --target=${NAME} flexsoc_cm3
      )
  endmacro( gw_target )

  function( gen_csr TARGET )
    add_custom_command(
      COMMENT "Generating CSR definitions"
      COMMAND ${FUSESOC_EXECUTABLE} --target=lint flexsoc_cm3
      COMMAND cmake -E make_directory ${PROJECT_BINARY_DIR}/generated
      COMMAND find ${CMAKE_CURRENT_BINARY_DIR}/build/flexsoc_cm3_0.1/lint-verilator/ -name '*.h' -exec cp {} ${PROJECT_BINARY_DIR}/generated \\\;
      COMMAND cmake -E touch ${PROJECT_BINARY_DIR}/csr_generated.txt
      OUTPUT  ${PROJECT_BINARY_DIR}/csr_generated.txt
      )
    add_custom_target( gen_csr
      DEPENDS ${PROJECT_BINARY_DIR}/csr_generated.txt
      )
    add_dependencies( ${TARGET} gen_csr )
    include_directories( ${PROJECT_BINARY_DIR}/generated )
    set_property( DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_BINARY_DIR}/csr_generated.txt )

  endfunction( gen_csr )
  
  macro( test_setup )
    # Decide where to run tests
    if( DEFINED ENV{FLEXSOC_HW} )
      set( FLEXSOC_HW $ENV{FLEXSOC_HW} )
      message( STATUS "Running on hw: ${FLEXSOC_HW}" )
    else ()
      set( FLEXSOC_HW "127.0.0.1:5555" )
      message( STATUS "Running on simulator: ${FLEXSOC_HW}" )
      # TODO: codify version if possible
      set( VERILATOR_SIM ${PROJECT_BINARY_DIR}/test/build/flexsoc_cm3_0.1/sim-verilator/Vflexsoc_cm3 )
      add_custom_command(
        OUTPUT ${VERILATOR_SIM}
        COMMENT "Generating verilated sim for ${CMAKE_PROJECT_NAME}"
        COMMAND ${FUSESOC_EXECUTABLE} --target=sim flexsoc_cm3
        )
    endif ()
    # Turn on testing
    enable_testing()
  endmacro( test_setup )

  macro( test_finalize )
    if( DEFINED ENV{FLEXSOC_HW} )
      add_custom_target( check
        COMMAND ${CMAKE_CTEST_COMMAND} )
    else ()
      add_custom_target( check
        DEPENDS ${VERILATOR_SIM}
        COMMAND ${CMAKE_CTEST_COMMAND} )
    endif ()
  endmacro( test_finalize )
  
  # Create test
  function( hw_test NAME SOURCES)
    add_executable( ${NAME} ${SOURCES} ${ARGN} )
    target_link_libraries( ${NAME} flexsoc target test )
    add_test(
      NAME ${NAME}
      COMMAND ${PROJECT_SOURCE_DIR}/test/scripts/hw_test.sh ${VERILATOR_SIM} $<TARGET_FILE:${NAME}> ${FLEXSOC_HW} )    
  endfunction( hw_test )

  # Create CLI tests
  function( cli_test NAME SYSMAP BINARY EXT )
    add_test(
      NAME ${NAME}
      COMMAND ${PROJECT_SOURCE_DIR}/test/scripts/cli_test.sh ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/test/map/${SYSMAP} ${PROJECT_SOURCE_DIR}/test/arm/bin/${BINARY} ${FLEXSOC_HW} ${EXT} ${VERILATOR_SIM}
      )
  endfunction( cli_test )
endif()
