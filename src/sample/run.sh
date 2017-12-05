#!/bin/bash

if [ $# != 2 ] ; then
    echo "USAGE: ./run.sh <cluster_name> <app_name>"
    echo "for example: ./run.sh onebox temp"
    exit 1;
fi

sed "s/@LOCAL_IP@/`hostname -i`/g" config-sample.ini > config.ini
export LD_LIBRARY_PATH=`pwd`/../../DSN_ROOT/lib
./pegasus_cpp_sample $1 $2

