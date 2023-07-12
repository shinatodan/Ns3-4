#set terminal postscript eps
set xrange [0.1:20.1]
set yrange [0:]
set xtics 1

set key right
set grid 

#nearint(x)=(x - floor(x) <= 0.5 ? floor(x) : floor(x)+1) 
nearint(x)=(x - floor(x) <= 0.5 ? (x - floor(x) <= 0 ? 0 : floor(x)) : floor(x)+1) 
filter(x,y)=nearint(x/y)*y 

set xlabel 'Times'
set ylabel 'cwnd(pkts)'
set title 'Tcp Congestion Window'
#set output 'exp03-CongestionWindows.eps'

plot "data/exp03-cwnd.tr" using 1:2 with line t 'cwnd' ,\
  "" u (filter($1,3)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average cwnd'

pause -1

set xlabel 'Times'
set ylabel 'kbps'
set title 'Tcp Throughputs at Sink Node'
#set output 'exp03-Throughputs.eps'

plot "data/exp03-throughput.tr" using 1:2 with line t 'throughput', \
        "" u (filter($1,3)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average throughput'

pause -1
