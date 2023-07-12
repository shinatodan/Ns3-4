#set terminal postscript eps
#set output 'RED-CongestionWindows.eps'
set xrange [5:180]

set key right
set grid 

nearint(x)=(x - floor(x) <= 0.5 ? floor(x) : floor(x)+1) 
filter(x,y)=nearint(x/y)*y 

set xlabel 'Times'
set ylabel 'pkts'
set title 'Congestion Window'

plot "data/exp07-n0.cwnd" using 1:2 with lines t 'cwnd(n0)' ,\
     "data/exp07-n1.cwnd" using 1:2 with lines t 'cwnd(n1)' ,\
     "data/exp07-n2.cwnd" using 1:2 with lines t 'cwnd(n2)' ,\
     "data/exp07-avg.cwnd" u (filter($1,6)):2 smooth csplines with linespoints pt 6 ps 1.0 t 'average'

pause -1

#set terminal postscript eps
#set output 'RED-queueSize.eps'
set title 'RED Queue Size of Bottleneck Link'

plot "data/exp07.qsize" using 1:2 with line t 'instant queue' lc rgb "green" lw 0.5,\
     "" u (filter($1,6)):2 smooth csplines with linespoints lc rgb "red" pt 6 ps 1.0 t 'average queue'

pause -1
