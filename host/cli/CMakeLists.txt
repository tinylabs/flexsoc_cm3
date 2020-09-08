#
# Host side flexsoc_cm3 CLI
#

include_directories( . plugins )

# Add install prefix to definitions
add_definitions( -DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}/plugins" )

# Create executable
add_executable( flexsoc_cm3
  flexsoc_cm3.cpp
  ll.c
  main.cpp
  plugini.cpp
  sysmap_parse.cpp
  )

# Link to comm lib
target_link_libraries( flexsoc_cm3 log target dl )
#set_target_properties( flexsoc_cm3 PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )

# Install into bin
install( TARGETS flexsoc_cm3
  DESTINATION bin )

# Add builtin plugin subdirectory
add_subdirectory( plugins )
