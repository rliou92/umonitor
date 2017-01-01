#!/bin/sh
clear
# rm umon.conf
# make clean && make && printf "\n" && ./display --save home
# make clean && make && printf "\n" && ./display --load home
 make clean && make && printf "\n" && ./display --test-event
