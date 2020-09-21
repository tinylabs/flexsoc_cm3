#
# CMake for flexsoc logging library
#


# Create target
add_library( log
  log.cpp
  )

# Enable debug
#set_target_properties( log PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )

# Install headers
install( FILES
  log.h
  DESTINATION include )
