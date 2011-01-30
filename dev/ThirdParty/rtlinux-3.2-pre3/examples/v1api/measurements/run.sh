#!/bin/bash

# script for starting the measurement example;
# example usage: ./run.sh minimize_jitter=0 period=300
# see rt_process.c for other parameters

make
rmmod rt_process
insmod rt_process $*
./monitor
