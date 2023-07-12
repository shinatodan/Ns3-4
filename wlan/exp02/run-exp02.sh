#!/bin/sh

echo "./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=100" > data/wexp02-th-dist-100"
./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=100" > data/wexp02-th-dist-100

echo "./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=150" > data/wexp02-th-dist-150"
./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=150" > data/wexp02-th-dist-150

echo "./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=200" > data/wexp02-th-dist-200"
./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=200" > data/wexp02-th-dist-200

echo "./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=220" > data/wexp02-th-dist-220"
./waf --run "exp02-simpleAdhoc --sendFlag=1 --distance=220" > data/wexp02-th-dist-220

echo "gnuplot plot-wexp02.pl"
gnuplot plot-wexp02.pl
