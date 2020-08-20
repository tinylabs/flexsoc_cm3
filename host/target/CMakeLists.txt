#
# CMake for flexsoc communication library
#


# Use host interface for autogen CSRs
add_definitions( -DHOST_INTERFACE=1 )

# Create target
add_library( target
  target.cpp
  )

# Enable debug
#set_target_properties( target PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )

# Add dependency to generate CSRs
gen_csr( target )

# Link to comm layer
target_link_libraries( target flexsoc )


