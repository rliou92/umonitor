#!/bin/bash
clear
# rm umon.conf
make clean && make && printf "\n" && ./display --save home
