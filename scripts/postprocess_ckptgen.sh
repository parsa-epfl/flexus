#!/bin/bash

MUNGE=/home/parsacom/tools/flexus/munge-checkpoint.py
SIMICS_DIR=/home/parsacom/tools/simics

SIMICS_BIN_DIR=`/bin/ls -d $SIMICS_DIR/*-linux/bin | head -n 1`

if [[ ( $# != 4 ) || ( $1 != "phase" && $1 != "flexpoint" ) ]]; then
	echo "usage: $0 phase|flexpoint <max-num> <flexstate> <retcode>"
	echo "	note that <retcode> is added by go.sh"
	exit -1
fi

LOG_FILE=gen-ckpt.log
# parameters: [options] msg retcode
log() {
	LOG_OPTS=
	while [[ ${1:0:1} = "-" ]]; do
		case $1 in 
			"-n" )
				LOG_OPTS="$LOG_OPTS -n"
				shift
				;;
			"-err" )
				LOG_ERR=1
				shift
				;;
		esac
	done 

	LOG_MSG=$1

	echo $LOG_OPTS $LOG_MSG
	echo $LOG_OPTS $LOG_MSG >> $LOG_FILE
	if [[ $LOG_ERR ]]; then
		if [[ $2 ]] ; then exit $2; else exit -1; fi
	fi
}

TYPE=$1
MAX_N=$2
FLEXSTATE=$3
SIMICS_RETCODE=$4

if [[ $SIMICS_RETCODE != 0 ]]; then
	log -err "ERROR: Invalid return code from Simics: $SIMICS_RETCODE" $SIMICS_RETCODE
fi

ORIGIN=`grep -E 'read-configuration /' job-load.simics | sed -e 's!read-configuration \(.*\)/simics/.*!\1!'`
if [[ `echo $ORIGIN | grep ${TYPE}_` = "" ]]; then
	N=1
else
	N=`echo $ORIGIN | perl -pi -e "s#.*/${TYPE}_(...).*#sprintf('%d',\\$1+1)#e"`
	ORIGIN=`dirname $ORIGIN`
fi
TYPE_N=${TYPE}_`printf '%03d' $N`
CKPT_BASE="$ORIGIN/$TYPE_N"

log -n "Confirm valid output... "
if [[ ! -e insn_count ]] ; then
	log -err "ERROR: couldn't find insn_count"
fi
if [[ ! -e flexus_state_out/simics-state ]] ; then
	log -err "ERROR: simics checkpoint not found"
fi
log "Done"

# if simics/ already exists, make sure it was created with same stop_cycle
if [[ -e $CKPT_BASE/simics/$TYPE_N ]]; then
	INSN_COUNT=`cat insn_count`
	INSN_COUNT_PREV=`cat $CKPT_BASE/simics/$TYPE_N.insn_count`
	if [[ "$INSN_COUNT" -ne "$INSN_COUNT_PREV" ]]; then
		log -err "ERROR: insn_count=$INSN_COUNT != previous=$INSN_COUNT_PREV"
	fi
else
	log -n "Need to copy simics-state... "
	mkdir -p $CKPT_BASE/simics
	if [[ ! -d $CKPT_BASE/simics ]]; then
		log -err "\n	ERROR: unable to create simics directory $CKPT_BASE/simics"
	fi

	log -n "remove flexus_ and dma_tracer references... "
	perl -ni -e "print unless /timing_model:/" flexus_state_out/simics-state
	log -n "moving... "
	perl -pi -e "s!\".*?flexus_state_out!\"$CKPT_BASE/simics!g;s!(\"|/)simics-state!\$1$TYPE_N!g;" flexus_state_out/simics-state
	for orig_name in flexus_state_out/simics-state*; do
		NEW_NAME=`echo $orig_name | sed -e "s!.*/simics-state\([^/]*\)!/$TYPE_N\\1!"`
		mv $orig_name $CKPT_BASE/simics/$NEW_NAME
	done
	mv insn_count $CKPT_BASE/simics/$TYPE_N.insn_count
	mv *console.out $CKPT_BASE/simics
	log -n "munge... "
	# the munge-checkpoint.py script needs craff to be in the path
	#	We should fix this so we can at least pass which craff to use as parameter
	env PATH=$SIMICS_BIN_DIR:$PATH $MUNGE $CKPT_BASE/simics/$TYPE_N > $CKPT_BASE/simics/$TYPE_N.munge_output || exit -1
	log "Done"
fi

if [[ $TYPE = "flexpoint" ]] ; then
	CKPT_FXPT_PATH="$CKPT_BASE/$FLEXSTATE"
	if [[ -d $CKPT_FXPT_PATH ]]; then
		while [[ -d $CKPT_FXPT_PATH ]]; do
			log "Flexpoint $CKPT_FXPT_PATH already exists, skipping..."
			N=$(($N+1))
			TYPE_N=${TYPE}_`printf '%03d' $N`
			CKPT_BASE="$ORIGIN/$TYPE_N"
			CKPT_FXPT_PATH="$CKPT_BASE/$FLEXSTATE"
		done
		# above loop overshoots by 1, go back to latest existing flexpoint
		N=$(($N-1))
		TYPE_N=${TYPE}_`printf '%03d' $N`
		CKPT_BASE="$ORIGIN/$TYPE_N"
		CKPT_FXPT_PATH="$CKPT_BASE/$FLEXSTATE"
		# pretend that we just finished running the latest existing flexpoint
		log -n "Restore state from $CKPT_FXPT_PATH... "
		rm -rf flexus_state_out
		mkdir flexus_state_out
		tar x -C flexus_state_out -f $CKPT_FXPT_PATH/flexstate.tar.gz
		log "Done"
	else
		log -n "Copy the flexpoint... "
		mkdir -p $CKPT_FXPT_PATH
		if [[ ! -d $CKPT_FXPT_PATH ]]; then
			log -err "\n	ERROR: unable to create checkpoint directory $CKPT_FXPT_PATH"
		fi
		tar zc --exclude simics-state\* -C flexus_state_out -f $CKPT_FXPT_PATH/flexstate.tar.gz .
		cp configuration.out $CKPT_FXPT_PATH/configuration.out
		log "Done"
	fi
fi

# is this the last requested flexpoint?
if [[ $N = $MAX_N ]]; then
	# clean up things that are not worth saving
	rm -rf flexus_state_out
	log -err "Checkpoint generation completed" 0
fi

log -n "Prepare flexstate for next flexpoint... "
rm -rf flexus_state_in
mv flexus_state_out flexus_state_in
log "Done"

log -n "Prepare job-load.simics for next flexpoint... "
perl -pi -e "s#read-configuration .*#read-configuration $CKPT_BASE/simics/$TYPE_N#" job-load.simics
log "Done"

if [[ $TYPE = "flexpoint" ]] ; then
	# if this flexpoint was without flexstate, start loading the flexstate for subsequent flexpoints
	grep flexus.load-state custom-postload.simics > /dev/null
	if [ $? -ne 0 ]; then
		log "Add flexus.load-state to custom-postload.simics"
		echo "flexus.load-state \"flexus_state_in\"" >> custom-postload.simics
	fi
fi

exec ./go.sh 
