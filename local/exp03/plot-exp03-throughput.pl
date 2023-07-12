#set terminal postscript eps
#set output 'Throughputs.eps'

set key right
set grid 
set xrange [0:20]

set xlabel 'Times'
set ylabel 'kbps'
set title 'Total Throughputs at Sink Node'

nearint(x)=(x - floor(x) <= 0.5 ? floor(x) : floor(x)+1) 
filter(x,y)=nearint(x/y)*y 

plot "data/exp03-TcpNewReno-Constant.throughput" using 1:2 with line t 'TcpNewReno', \
	"" u (filter($1,5)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average'

pause -1

plot "data/exp03-TcpReno-Constant.throughput" using 1:2 with line t 'TcpReno', \
	"" u (filter($1,5)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average'

pause -1

plot "data/exp03-TcpTahoe-Constant.throughput" using 1:2 with line t 'TcpTahoe', \
	"" u (filter($1,5)):2 smooth csplines with linespoints pt 6 ps 1.2 t 'average'

pause -1
