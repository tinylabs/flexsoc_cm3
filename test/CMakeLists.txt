#
# System level tests
#

# Setup test framework
test_setup()

# Create test library
add_library( test common.cpp )

# Add tests here...
hw_test( test-master master_test.cpp)
hw_test( test-slave slave_test.cpp )
hw_test( test-latency latency.cpp )
hw_test( test-throughput throughput.cpp )
#hw_test( io_test io_test.cpp )

#set_target_properties( test-throughput PROPERTIES COMPILE_FLAGS "-O0 -ggdb")
#set_target_properties( io_test PROPERTIES COMPILE_FLAGS "-O0 -ggdb")

# Finalize testing
test_finalize()
