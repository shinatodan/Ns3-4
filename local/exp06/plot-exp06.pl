#set terminal postscript eps
#set output 'CongestionWindows.eps'
set xrange [10:150]
set yrange [10:20000]
set key right
set grid 

nearint(x)=(x - floor(x) <= 0.5 ? floor(x) : floor(x)+1) 
filter(x,y)=nearint(x/y)*y 

set xlabel 'Times'
set ylabel 'pkts'
set title 'Congestion Window'

plot "data/exp06-n0.cwnd" using 1:2 with lines t 'cwnd(n0)' ,\
     "data/exp06-n1.cwnd" using 1:2 with lines t 'cwnd(n1)' ,\
     "data/exp06-n2.cwnd" using 1:2 with lines t 'cwnd(n2)' ,\
     "data/exp06-avg.cwnd" u (filter($1,6)):2 smooth csplines with linespoints pt 6 ps 1.0 t 'average'
     
pause -1

