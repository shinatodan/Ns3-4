#!/bin/sh

gnuplot plot-exp08.pl 

python mk-throughput-graph.py

gnuplot plot-throughput-utility.pl
