#!/bin/bash
#
# Connect to hardware or simulation to run basic tests
#

# Debug
#set -x

# Decode arguments
if [ $# -eq 2 ]; then
    TEST=$1
    HW=$2
elif [ $# -eq 3 ]; then
    SIM=$1
    TEST=$2
    HW=$3
    HOST_PORT=($(echo $3 | tr ':' ' '))
    HOST=${HOST_PORT[0]}
    PORT=${HOST_PORT[1]}
else
    echo "Incorrect arguments passed!"
    exit -1
fi

set -m

# Start simulator
if [ -n $SIM ]; then
#    $SIM -u$PORT --vcd=sim.vcd &
    $SIM -u$PORT &
    SIM_PID=$!
    sleep 0.5
fi

# Run test
$TEST $HW
RV=$?

# Kill sim if running
if [ -n $SIM_PID ]; then
    kill -9 $SIM_PID
fi

# Return test result
exit $RV
