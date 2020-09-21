#
# Builtin flexsoc_cm3 plugins - These can be extended with other compiled plugins
#

# Create helper lib
add_library( pluginhelper STATIC pluginhelper.cpp )
#set_target_properties( pluginhelper PROPERTIES COMPILE_FLAGS "-fPIC -O0 -ggdb" )
set_target_properties( pluginhelper PROPERTIES COMPILE_FLAGS "-fPIC" )

# Install helper lib
install( TARGETS
  pluginhelper
  DESTINATION lib )
set_property( GLOBAL APPEND PROPERTY PRIVATE_LIBS "-lpluginhelper" )

# Install helper header
install( FILES
  plugin.h
  pluginhelper.h
  BusPeripheral.h
  DESTINATION include )

# Builtin plugins
plugin( Memory Memory.cpp )
plugin( UnitTest UnitTest.cpp )
plugin( Redirect Redirect.cpp )

# Install plugins
install( TARGETS
  Memory UnitTest Redirect
  DESTINATION plugins )

