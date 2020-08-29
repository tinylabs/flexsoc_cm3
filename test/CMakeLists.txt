#
# System level tests
#

if( CMAKE_CROSSCOMPILING )

  # Add ARM tests
  # compile by invoking cmake -DBUILD_ARM=1 ../ from fresh build_arm directory
  add_subdirectory( arm )

else( NOT CMAKE_CROSSCOMPILING )
  
  # Setup test framework
  test_setup()
  
  # Add hardware tests
  add_subdirectory( hw )

  # Add cli tests
  add_subdirectory( cli )
  
  # Finalize testing
  test_finalize()

endif()
