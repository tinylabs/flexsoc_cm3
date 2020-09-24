#
# Host side flexsoc_cm3 CLI
#

include_directories( . plugins )

# Add install prefix to definitions
add_definitions( -DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}/plugins" )

# Create executable
add_executable( flexsoc-cm3
  flexsoc_cm3.cpp
  ll.c
  main.cpp
  plugini.cpp
  remote.cpp
  sysmap_parse.cpp
  )

# Link to comm lib
target_link_libraries( flexsoc-cm3 log target dl )
#set_target_properties( flexsoc-cm3 PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )

# Install into bin
install( TARGETS flexsoc-cm3
  DESTINATION bin )

# Add builtin plugin subdirectory
add_subdirectory( plugins )
