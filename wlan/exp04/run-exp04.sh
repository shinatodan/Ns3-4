#!/bin/sh

./waf --run "exp04-simpleAdhocWithEnergy --sendFlag=1 --traceNode=0 --traceFlag=remained"|tee data/energy-node-0

gnuplot plot-wexp04.sh
