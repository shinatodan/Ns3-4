#!/bin/sh

#python mkaverage.py > data/exp07-avg.cwnd
python mkaverage.py
gnuplot plot-exp07.pl
