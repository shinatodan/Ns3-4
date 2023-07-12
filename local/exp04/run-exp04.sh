#!/bin/sh

./waf --run exp04-TcpFlowMonitoring
python mkhistgram.py jitterHistogram data/exp04-TCP-flowmon.xml
python mkhistgram.py delayHistogram data/exp04-TCP-flowmon.xml
gnuplot plot-histgram.pl

