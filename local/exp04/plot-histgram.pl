#set terminal postscript eps
#set output 'jitterHist.eps'

set xlabel 'Times'
set ylabel 'pkts'
set title 'Histogram of the packet jitters'

set boxwidth 0.001
set grid

plot 'data/jitterHistogram-0.tr' using 1:2 w boxes fs s 0.5 lt 1 t 'n1-n4'
pause -1
plot 'data/jitterHistogram-1.tr' using 1:2 w boxes fs s 0.5 lt 1 t 'n2-n5'
pause -1
plot 'data/jitterHistogram-2.tr' using 1:2 w boxes fs s 0.5 lt 1 t 'n3-n6'
pause -1

set title 'Histogram of the packet delays'
#set output 'delayHist.eps'

plot 'data/delayHistogram-0.tr' using 1:2 w boxes fs s 0.5 lt 1 t 'n1-n4'
pause -1
plot 'data/delayHistogram-1.tr' using 1:2 w boxes fs s 0.5 lt 1 t 'n2-n5'
pause -1
plot 'data/delayHistogram-2.tr' using 1:2 w boxes fs s 0.5 lt 1 t 'n3-n6'
pause -1
