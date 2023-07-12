#!/bin/sh

./waf --run "exp06-simpleMesh --showRtable=1"

./procreport.pl data/simple-hwmp-report.xml
