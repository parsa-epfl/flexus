#!/bin/bash

OLD=${PWD}
SIM=${1:-FrisckyKraken}
NEW=build.${SIM}

mkdir -p ${NEW} && cd ${NEW}

exec cmake ${OLD}              \
    -DSIMULATOR=${SIM}         \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-DFLEXUS -Wno-error=maybe-uninitialized -Wno-error=dangling-pointer -Wno-error=array-bounds"
