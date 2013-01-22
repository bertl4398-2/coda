#!/bin/bash
PUB_PER_NODE=1
BSIZE=11
REQ_PER_NODE="5 5 5 10 10 10 15 15 15 20 20 20 25 25 25 30 30 30"
SET_ENV=$PWD/test_build_ns3_dce/ns-3-dce/utils/setenv.sh
PROG=$PWD/test_build_ns3_dce/build/bin/dce-ccnd-mobility-random4
COMBINE=$PWD/ccnx-0.6.0/bin/combinestats

DIR=$PWD
source $SET_ENV
cd /
cd $DIR

for i in $REQ_PER_NODE
do
    TSTAMP=`date +%Y%m%d%H%M%S`
    TEST_DIR=$PWD/testcoda_nodes50_reqs${i}_${TSTAMP}
    DATA_FILE="../data_nodes50_reqs${i}_ts${TSTAMP}.csv"
    OUTPUT_SIM="../simulation_nodes50_reqs${i}_ts${TSTAMP}.txt"

    if [ -d $TEST_DIR ]
    then
	cd $TEST_DIR
	rm -rf *
    else
	mkdir $TEST_DIR
	cd $TEST_DIR
    fi

#    BSIZE=$(echo "${PUB_PER_NODE}+${i}+5" | bc -l)

    PARAM="--numNodes=50 --simulationTime=2000 --phyMode=DsssRate1Mbps --verbose=0 --tracing=0 --width=500 --timeChange=2 --speed=Constant:13.89 --publicationsPerNode=$PUB_PER_NODE --requestsPerNode=$i --startPublication=50 --startRequest=60 --contentLifetime=2000 --requestLifetime=30 --sizeBuffer=$BSIZE --interRequestPeriod=0 --helloPeriodicity=1000000 --helloMaxBackoff=3 --pitDelay=500000 --pitMaxBackoff=3 --powerLaw=0.6667"

    $PROG --ccnxMode=1 $PARAM &>> $OUTPUT_SIM
    $COMBINE
    sort --version-sort < "combined.csv" > $DATA_FILE
    rm -rf *
    cd ..
    rmdir $TEST_DIR

done
