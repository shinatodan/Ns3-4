#set terminal postscript eps
#set output 'CongestionWindows.eps'
set xrange [0:20]

set key right
set grid 

set xlabel 'Times'
set ylabel 'throughput(bps)'
set title 'Throughputs vs. Distances'


plot "data/wexp02-th-dist-100" using 1:2 with linespoints pt 7  ps 0.7 t 'distance=100',\
     "data/wexp02-th-dist-150" using 1:2 with linespoints pt 9  ps 0.7 t 'distance=150', \
     "data/wexp02-th-dist-200" using 1:2 with linespoints pt 13 ps 0.7 t 'distance=200', \
     "data/wexp02-th-dist-220" using 1:2 with linespoints pt 13 ps 0.7 t 'distance=220'
     
pause -1
