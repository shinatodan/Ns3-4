#!/bin/sh

echo "Simulation for Tcp Tahoe ------------------------------"
./waf --run "exp02-BulkTCP-with-cwnd-trace --tcpType=TcpTahoe"

echo "Simulation for Tcp Reno -------------------------------"
./waf --run "exp02-BulkTCP-with-cwnd-trace --tcpType=TcpReno"

echo "Simulation for Tcp NewReno ----------------------------"
./waf --run "exp02-BulkTCP-with-cwnd-trace --tcpType=TcpNewReno"

#./waf --run "exp02-OnOffTCP-with-cwnd-trace --tcpType=TcpTahoe"
#./waf --run "exp02-OnOffTCP-with-cwnd-trace --tcpType=TcpReno"
#./waf --run "exp02-OnOffTCP-with-cwnd-trace --tcpType=TcpNewReno"

gnuplot plot-exp02-cwnd.pl

