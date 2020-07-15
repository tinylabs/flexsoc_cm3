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


# Finalize testing
test_finalize()
