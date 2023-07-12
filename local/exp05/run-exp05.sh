#!/bin/sh

./waf --run "exp05-ReceiveWithNoise --maxSize=10 --errorRate=0.000015"
#./waf --run "exp05-ReceiveWithNoise --maxSize=10 --errorRate=0.0"

gnuplot plot-exp05.pl
