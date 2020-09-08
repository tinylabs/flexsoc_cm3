#
# CMake for flexsoc communication library
#

# Include libftdi
find_package( LibFTDI1 NO_MODULE REQUIRED )
include( ${LIBFTDI_USE_FILE} )

add_library( flexsoc
  flexsoc.cpp
  FTDITransport.cpp
  TCPTransport.cpp
  Cbuf.cpp
  )

target_link_libraries( flexsoc log pthread ${LIBFTDI_LIBRARIES} )

# Enable debug
#set_target_properties( flexsoc PROPERTIES COMPILE_FLAGS "-O0 -ggdb" )
