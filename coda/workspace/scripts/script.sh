#!/bin/bash
PUB_PER_NODE=1
REQ_PER_NODE=5
BSIZE=$(echo "${PUB_PER_NODE}+${REQ_PER_NODE}+5" | bc -l)
ZIPF="0.0 0.0 0.0 0.3333 0.3333 0.3333 0.6667 0.6667 0.6667 1.0 1.0 1.0 1.3333 1.3333 1.3333 1.6667 1.6667 1.6667 2.0 2.0 2.0"
SET_ENV=$PWD/test_build_ns3_dce/ns-3-dce/utils/setenv.sh
PROG=$PWD/test_build_ns3_dce/build/bin/dce-ccnd-mobility-random4
COMBINE=$PWD/ccnx-0.6.0/bin/combinestats

DIR=$PWD
source $SET_ENV
cd /
cd $DIR

for i in $ZIPF
do
    TSTAMP=`date +%Y%m%d%H%M%S`
    TEST_DIR=$PWD/testcoda_nodes50_zipf${i}_${TSTAMP}
    DATA_FILE="../data_nodes50_zipf${i}_ts${TSTAMP}.csv"
    OUTPUT_SIM="../simulation_nodes50_zipf${i}_ts${TSTAMP}.txt"

    if [ -d $TEST_DIR ]
    then
	cd $TEST_DIR
	rm -rf *
    else
	mkdir $TEST_DIR
	cd $TEST_DIR
    fi

    PARAM="--numNodes=50 --simulationTime=2000 --phyMode=DsssRate1Mbps --verbose=0 --tracing=0 --width=500 --timeChange=2 --speed=Constant:13.89 --publicationsPerNode=$PUB_PER_NODE --requestsPerNode=$REQ_PER_NODE --startPublication=50 --startRequest=60 --contentLifetime=2000 --requestLifetime=30 --sizeBuffer=$BSIZE --interRequestPeriod=0 --helloPeriodicity=1000000 --helloMaxBackoff=3 --pitDelay=500000 --pitMaxBackoff=3 --powerLaw=$i"

    $PROG --ccnxMode=2 $PARAM &>> $OUTPUT_SIM
    $COMBINE
    sort --version-sort < "combined.csv" > $DATA_FILE
    rm -rf *
    cd ..
    rmdir $TEST_DIR

done
