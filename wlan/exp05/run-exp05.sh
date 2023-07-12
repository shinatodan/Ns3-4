#!/bin/sh

echo ./waf --run "exp05-simpleRouting --numPackets=1000 --rtProto=AODV"
./waf --run "exp05-simpleRouting --numPackets=1000 --rtProto=AODV"
