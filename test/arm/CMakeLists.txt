#
# Compile ARM tests
#

add_library( arm OBJECT common.S common.c )

# Generate binary from source
include_directories( . )
add_subdirectory( src )
