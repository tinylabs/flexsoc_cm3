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
  )

target_link_libraries( flexsoc pthread ${LIBFTDI_LIBRARIES} )
