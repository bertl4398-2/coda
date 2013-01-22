#!/bin/bash
# this script builds NS3 and DCE.
cd `dirname $BASH_SOURCE`/../..
SAVE_PATH=$PATH
SAVE_LDLP=$LD_LIBRARY_PATH
SAVE_PKG=$PKG_CONFIG_PATH
#echo clone ns-3-dce : 
#hg clone http://code.nsnam.org/furbani/ns-3-dce
cd ns-3-dev
#CXXFLAGS="-O3" ./waf configure -d optimized --prefix=`pwd`/../build --enable-tests
./waf configure -d debug --prefix=`pwd`/../build --enable-tests --enable-examples
./waf
./waf install
cd ..
export PATH=$SAVE_PATH:`pwd`/build/bin
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib
export PKG_CONFIG_PATH=$SAVE_PKG:`pwd`/build/lib/pkgconfig
cd readversiondef/
make 
make install PREFIX=`pwd`/../build/
cd ..
cd ns-3-dce/
./waf configure --prefix=`pwd`/../build --verbose $WAF_KERNEL
./waf
./waf install
cd build/bin
ln -s ../../../../ccnx-0.6.0/bin/* .
cd ../..
export LD_LIBRARY_PATH=$SAVE_LDLP:`pwd`/build/lib:`pwd`/build/bin:`pwd`/../build/lib
. utils/setenv.sh
echo Launch NS3TEST-DCE
./build/bin/ns3test-dce --verbose
