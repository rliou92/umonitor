#!/bin/sh
clear
# rm umon.conf
make clean && make && printf "\n" && ./display --save docked --verbose
# make clean && make && printf "\n" && ./display --load home --verbose
# make clean && make && printf "\n" && ./display --test-event --verbose
