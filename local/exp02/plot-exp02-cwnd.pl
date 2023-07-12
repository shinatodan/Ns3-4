#set terminal postscript eps
#set output 'exp02-cwnd.eps'
set xrange [1:10]
set yrange [0:10000]

set key right
set grid 

set xlabel 'Times'
set ylabel 'cwnd(pkts)'
set title 'Congestion Window'

plot "data/TcpNewReno-bulk.cwnd" using 1:2 \
	with linespoints pt 7 ps 0.9 t 'TcpNewReno', \
	"data/TcpReno-bulk.cwnd" using 1:2 with linespoints pt 9 ps 0.9 t 'TcpReno', \
	"data/TcpTahoe-bulk.cwnd" using 1:2 with linespoints pt 13 ps 0.9 t 'TcpTahoe' 
     
pause -1
