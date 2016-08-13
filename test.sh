#!/bin/bash
clear
# rm umon.conf
make clean && make && ./display --save home
