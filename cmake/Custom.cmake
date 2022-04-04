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

endif()
