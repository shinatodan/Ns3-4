#!/bin/sh

echo "Simulation for Constant flow ----------------------------"
./waf --run "exp03-EstimateThroughput --tcpType=TcpNewReno --flowType=Constant"
./waf --run "exp03-EstimateThroughput --tcpType=TcpReno --flowType=Constant"
./waf --run "exp03-EstimateThroughput --tcpType=TcpTahoe --flowType=Constant"

#echo "Simulation for Exponential flow -------------------------------"
#./waf --run "exp03-EstimateThroughput --tcpType=TcpNewReno --flowType=Exponential"
#./waf --run "exp03-EstimateThroughput --tcpType=TcpReno --flowType=Exponential"
#./waf --run "exp03-EstimateThroughput --tcpType=Tahoe --flowType=Exponential"

#echo "Simulation for Uniform flow ------------------------------"
#./waf --run "exp03-EstimateThroughput --tcpType=TcpNewReno --flowType=Uniform"
#./waf --run "exp03-EstimateThroughput --tcpType=reno --flowType=Uniform"
#./waf --run "exp03-EstimateThroughput --tcpType=TcpTahoe --flowType=Uniform"

echo "Show Results -- Congestion Windows ----------------------------"
gnuplot plot-exp03-cwnd.pl

echo "Show Results -- Throutghputs ----------------------------------"
gnuplot plot-exp03-throughput.pl

