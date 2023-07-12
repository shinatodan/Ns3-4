#set terminal postscript eps
#set output 'th-uti.eps'

set xlabel 'delay(ms)'
set ylabel 'throughput(kbps)'
set title 'Average Throughputs and Link Utilities'
set xrange [0:53]
set yrange [0:]
set style fill border 1 
set style fill solid 0.1 

set y2range [0:1]
set y2tics 0, 0.2
set ytics nomirror
set y2label 'Utilities'

set boxwidth 4.00
set grid

plot 'data/exp08.th' using 1:2 w boxes fs s 0.5 lt 1 t 'throughputs',\
     "" u 1:3 axis x1y2 w linespoints pt 7 ps 1.5 t 'link utilities'
pause -1

