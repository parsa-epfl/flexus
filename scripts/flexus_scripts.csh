set BASE = /SET/PATH/TO/FLEXUS_ROOT
setenv PATH ${BASE}/gcc/bin:${PATH}:${BASE}/scripts:${BASE}/stat-manager
setenv TCLX_LIBRARY ${BASE}/tclx/lib/tclx8.4
setenv LD_LIBRARY_PATH ${BASE}/gcc/lib
source ${BASE}/condor/condor.csh
