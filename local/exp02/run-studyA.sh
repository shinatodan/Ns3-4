#!/bin/sh

./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=500Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=1000Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=1500Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=2000Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=2500Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=3000Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=3500Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=4000Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=4500Kbps"
./waf --run "exp02-studyA --tcpType=TcpNewReno --bandwidth=5000Kbps"

