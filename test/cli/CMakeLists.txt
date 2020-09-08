#
# The ARM portion must be compiled separately to generate the binaries.
# The tests however run anytime we run 'make check'
#

cli_test( test-arm-sanity empty.map arm-sanity.bin 0 )
cli_test( test-plugin-memory memory.map plugin-memory.bin 0 )
cli_test( test-plugin-redirect redirect.map plugin-redirect.bin 0 )
