#!/bin/sh

./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=05 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=10 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=15 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=20 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=25 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=30 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=35 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=40 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=45 --duration=100"
./waf --run "exp08-LongFatPipeTcp --bw=5Mbps --dl=50 --duration=100"
