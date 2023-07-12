#set terminal postscript eps
#set output 'CongestionWindows.eps'
set xrange [0:20]

set key right
set grid 

set xlabel 'Times'
set ylabel 'cwnd(pkts)'
set title 'Congestion Window'

plot "data/exp03-TcpNewReno-Constant.cwnd" using 1:2 with linespoints pt 7 ps 0.9 t 'TcpNewReno',\
	"data/exp03-TcpReno-Constant.cwnd" using 1:2 with linespoints pt 9 ps 0.9 t 'TcpReno', \
	"data/exp03-TcpTahoe-Constant.cwnd" using 1:2 with linespoints pt 13 ps 0.9 t 'Tcptahoe' 
     
pause -1
