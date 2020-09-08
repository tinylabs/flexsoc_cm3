#
# Builtin flexsoc_cm3 plugins - These can be extended with other compiled plugins
#

# Create helper lib
add_library( pluginhelper STATIC pluginhelper.cpp )
#set_target_properties( pluginhelper PROPERTIES COMPILE_FLAGS "-fPIC -O0 -ggdb" )
set_target_properties( pluginhelper PROPERTIES COMPILE_FLAGS "-fPIC" )

# Builtin plugins
plugin( Memory Memory.cpp )
plugin( UnitTest UnitTest.cpp )
plugin( Redirect Redirect.cpp )
