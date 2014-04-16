#!/bin/bash

if [ -z "$*" ]; then
	echo "Specify job directories to sample, e.g.:"
	echo "  `basename $0` db2v8_tpcc_4cpu_64cl oracle_4cpu_16cl"
	exit
fi

for j in $*; do
	pushd $j
	( stat-sample stats_db.out.gz */stats_db.out.selected.gz ; grep -vE '^$' */UIPC | sed 's/UIPC: //' | sort > UIPCs ) &
	popd
done
wait
