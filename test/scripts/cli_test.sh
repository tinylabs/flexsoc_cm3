#!/bin/bash
#
# Wrapper for CLI test
#

# Debug
set -x

# Check args
if [ $# -lt 5 ]; then
    echo "Must pass BINROOT, SYSMAP, BINARY, HWID, EXT [,SIM]"
    exit -1
fi

# Store args
ROOT=$1
MAP=$2
BIN=$3
HWID=$4
EXT=$5

# If extra argument it must be simulator
if [ $# -eq 6 ]; then
    SIM=$6

    # Extract port
    HOST_PORT=($(echo $4 | tr ':' ' '))
    HOST=${HOST_PORT[0]}
    PORT=${HOST_PORT[1]}
    
    # Start sim
    #$SIM -u$PORT --vcd=sim.vcd &
    $SIM -u$PORT &
    SIM_PID=$!
    sleep 0.5
fi

# Run CLI
if [[ $EXT == "1" ]]; then
    echo "External interface not supported yet"
else
    $ROOT/host/cli/flexsoc_cm3 -p $ROOT/host/cli/plugins -m $MAP -l $BIN $HWID
fi
RV=$?

# Kill simulator
if [ -n "$SIM_PID" ]; then
    kill -9 $SIM_PID
fi

exit $RV
