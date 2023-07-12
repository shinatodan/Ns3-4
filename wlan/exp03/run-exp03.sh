#!/bin/sh

./waf --run "exp03-infraTCP --distance=80 --lossType=log"
gnuplot plot-exp03.pl
