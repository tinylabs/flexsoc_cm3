#
# Find fusesoc installation
#
# All rights reserved.
# Tiny Labs Inc
#

find_program( FUSESOC_EXECUTABLE fusesoc
  HINTS
  ~/.local/bin
  DOC "Path to fusesoc executable")

# Override to point somewhere else
set( FUSESOC_COREROOT "." )

mark_as_advanced( FUSESOC_EXECUTABLE )
find_package( PackageHandleStandardArgs REQUIRED )
find_package_handle_standard_args( fusesoc REQUIRED_VARS
  FUSESOC_EXECUTABLE FUSESOC_COREROOT )

set( FUSESOC_FOUND ${FUSESOC_FOUND} )
set( FUSESOC_EXECUTABLE ${FUSESOC_EXECUTABLE} --cores-root ${FUSESOC_COREROOT} run )
