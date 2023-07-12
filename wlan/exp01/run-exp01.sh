#!/bin/sh

echo "./waf --run "exp01-hidden-terminal --CtsRts=0""
./waf --run "exp01-hidden-terminal --CtsRts=0"

echo "./waf --run "exp01-hidden-terminal --CtsRts=1""
./waf --run "exp01-hidden-terminal --CtsRts=1"
