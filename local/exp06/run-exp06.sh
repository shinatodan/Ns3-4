#!/bin/sh

./waf --run "exp06-GlobalSynchro --randStart=1"
python mkaverage.py
gnuplot plot-exp06.pl 
