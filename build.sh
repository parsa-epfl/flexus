#!/bin/bash

SIM=${1:-KeenKraken}

mkdir -p ${SIM} 

exec cmake -B ${SIM} \
    -DSIMULATOR=${SIM} \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_FLAGS="-DFLEXUS -DTEST_VMA -Wno-error=maybe-uninitialized -Wno-error=dangling-pointer"
