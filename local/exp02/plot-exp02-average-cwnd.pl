#set terminal postscript eps
#set output 'exp02-avg-cwnd.eps'
set xrange [1:10]
set yrange [0:]

set key right
set grid 

set xlabel 'Times'
set ylabel 'cwnd(pkts)'
set title 'Congestion Window'

nearint(x)=(x - floor(x) <= 0.5 ? floor(x) : floor(x)+1) 
filter(x,y)=nearint(x/y)*y 

plot "data/TcpNewReno-bulk.cwnd" using 1:2 with linespoints pt 6 ps .6 t 'TcpNewReno' ,\
  "" u (filter($1,2)):2 smooth csplines with linespoints pt 13 ps 1.2 t 'average cwnd'
     
pause -1
