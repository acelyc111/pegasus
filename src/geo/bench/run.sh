#!/bin/bash
for level in 13 14 15 16 17 18 19 20
do
    for radius in 50 100 200 300 500 1000 2000 5000 8000
    do
        echo "./pegasus_geo_bench c4tst-geo cell_position_data cell_position_data_geo $radius 2000 $level"
        ./pegasus_geo_bench c4tst-geo cell_position_data cell_position_data_geo $radius 2000 $level
        sleep 5
    done
done
