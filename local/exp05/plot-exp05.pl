#set terminal postscript eps
#set output 'CongestionWindows.eps'
set xrange [0:20]

set key right
set grid 

nearint(x)=(x - floor(x) <= 0.5 ? floor(x) : floor(x)+1) 
filter(x,y)=nearint(x/y)*y 

set xlabel 'Times'
set ylabel 'cwnd(pkts)'
set title 'Congestion Window'

plot "data/exp05.cwnd" using 1:2 with line t 'instant cwnd' ,\
  "" u (filter($1,5)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average cwnd'

pause -1

set xlabel 'Times'
set ylabel 'kbps'
set title 'Total Throughputs at Sink Node'

plot "data/exp05.throughput" using 1:2 with line t 'instant throughput', \
        "" u (filter($1,5)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average throughput'

pause -1

set xlabel 'Times'
set ylabel 'pkts'
set title 'DropTail Queue Size of Bottleneck Lins'

plot "data/exp05.qsize" using 1:2 with line t 'instant queue size' ,\
  "" u (filter($1,5)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average queue size'

pause -1
