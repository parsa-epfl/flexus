BASE=/SET/PATH/TO/FLEXUS_ROOT
export PATH=$BASE/gcc/bin:$PATH:$BASE/scripts:$BASE/stat-manager
export TCLX_LIBRARY=$BASE/tclx/lib/tclx8.4
export LD_LIBRARY_PATH=$BASE/gcc/lib
source $BASE/condor/condor.sh
