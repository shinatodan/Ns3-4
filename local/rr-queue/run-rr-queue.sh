#!/bin/sh

./waf --run "rr-queue-example -queueType=DropTail"
echo "============================================================="
./waf --run "rr-queue-example -queueType=RR"
