#!/bin/bash

NODES=50 # number of nodes
PUB_PER_NODE=1 # publications per node
REQ_PER_NODE=15 # requests per node
BSIZE=11 # buffer size per node (content items)
ZIPF="0.8" # Zipf distribution parameter
WIDTH=500 # width of the squared area (m)
TIME_CHANGE=2 # time change for random direction (sec)
TTL=30 # request lifetime (sec) (max. 90 can be changed in ccnpeek)
IR_PERIOD=0 # time betwee requests (sec)
HELLO_PERIOD=1000000 # HELLO period (musec)
PIT_DELAY=500000 # PIT delay (musec)
SPEED="2" # constant speed for random direction (m/s)
TXGAIN=-14 # transmission power gain reduction (dB)
RXGAIN=-10 # receiving power gain reduction (dB)
TRACE=$PWD/rwdata.ns_movements # set to "random" for random direction 
OUT=. # name of the output folder

SET_ENV=$PWD/test_build_ns3_dce/ns-3-dce/utils/setenv.sh
PROG=$PWD/test_build_ns3_dce/build/bin/dce-ccnd-mobility-random4
COMBINE=$PWD/ccnx-0.6.0/bin/combinestats

DIR=$PWD
source $SET_ENV
cd /
cd $DIR

TSTAMP=`date +%Y%m%d%H%M%S`
HOST=`hostname`
TEST_DIR=/tmp/testcoda_${TSTAMP}
DATA_FILE=$PWD/"${OUT}/data_${TSTAMP}.csv"
OUTPUT_SIM=$PWD/"${OUT}/simulation_${TSTAMP}.txt"

if [ -d $TEST_DIR ]
then
    cd $TEST_DIR
    rm -rf *
else
    mkdir $TEST_DIR
    cd $TEST_DIR
fi
# simulation parameters
PARAM="--numNodes=$NODES --simulationTime=4000 --txgain=$TXGAIN --rxgain=$RXGAIN --phyMode=DsssRate1Mbps --verbose=0 --tracing=0 --width=$WIDTH --timeChange=$TIME_CHANGE --speed=Constant:$SPEED --publicationsPerNode=$PUB_PER_NODE --requestsPerNode=$REQ_PER_NODE --startPublication=50 --startRequest=60 --contentLifetime=4000 --requestLifetime=$TTL --sizeBuffer=$BSIZE --interRequestPeriod=$IR_PERIOD --helloPeriodicity=$HELLO_PERIOD --helloMaxBackoff=3 --pitDelay=$PIT_DELAY --pitMaxBackoff=3 --powerLaw=$ZIPF --trace=$TRACE"
#############################################
# ccnxMode=0 LRU (ccnx base mode)
# ccnxMode=1 LRUMRU (not fully implemented)
# ccnxMode=2 CODA
#############################################
$PROG --ccnxMode=2 $PARAM >> $OUTPUT_SIM 2>&1
$COMBINE

sort --version-sort < "combined.csv" > $DATA_FILE

rm -rf *
cd ..
rmdir $TEST_DIR

exit
