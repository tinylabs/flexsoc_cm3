#
# System level tests
#

# Create test library
add_library( test common.cpp )

# Add tests here...
hw_test( test-master master.cpp)
hw_test( test-slave slave.cpp )
hw_test( test-latency latency.cpp )
hw_test( test-throughput throughput.cpp )

#set_target_properties( test-latency PROPERTIES COMPILE_FLAGS "-O0 -ggdb")
