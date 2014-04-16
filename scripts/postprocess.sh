#!/bin/bash

# only post-process timing jobs
grep -- -ma go.sh > /dev/null || exit

SCRIPT_DIR=`dirname $0`

# dummy is needed to prevent clobbering debug.out and friends
mkdir -p dummy
cd dummy
source $SCRIPT_DIR/flexus_scripts.sh
$SCRIPT_DIR/stat-collapse 100000 149999 ../stats_db.out.gz ../stats_db.out.selected.gz
$SCRIPT_DIR/stat-manager -d ../stats_db.out.selected.gz format-string "UIPC: <EXPR:{Nodes-uarch-TB:User:Commits:Busy}/({Nodes-uarch-TB:User:AccountedCycles}+{Nodes-uarch-TB:System:AccountedCycles})*1000>" "selection" | grep UIPC > ../UIPC
