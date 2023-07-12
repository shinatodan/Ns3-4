#!/bin/sh

#./waf --run "exp07-Red-Droptail --queueType=RED"
./waf --run "exp07-Red-Droptail --queueType=DropTail"

python mkaverage.py
gnuplot plot-exp07.pl
