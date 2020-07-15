#
# System level tests
#

# Setup test framework
test_setup()

# Add tests here...
host_test( test-master master_test.cpp)
host_test( test-slave slave_test.cpp )


# Finalize testing
test_finalize()
