#!/bin/sh

TOOLCHAIN_BASE=/mnt/hgfs/scratch/deps/toolchain-5.2
export PATH=$TOOLCHAIN_BASE/bin:$PATH
export LD_LIBRARY_PATH=$TOOLCHAIN_BASE/lib:$TOOLCHAIN_BASE/lib64:/usr/lib32:$LD_LIBRARY_PATH
