#
# Host side flexsoc_cm3 CLI
#

include_directories( . plugin )

# Create executable
add_executable( flexsoc_cm3
  flexsoc_cm3.cpp
  main.cpp
  )

# Link to comm lib
target_link_libraries( flexsoc_cm3 flexsoc )
#set_target_properties( flexsoc_cm3 PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )

# Add builtin plugin subdirectory
add_subdirectory( plugin )
