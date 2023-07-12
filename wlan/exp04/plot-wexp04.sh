#set terminal postscript eps
#set output 'CongestionWindows.eps'

set key right
set grid 

set xlabel 'Times'
set ylabel 'Joule'
set title  'Energy'

#plot "data/energy-node-0" using 1:2 with linespoints pt 7 ps 0.5 t 'Energy'
plot "data/energy-node-0" using 1:2 with lines t 'Energy'
     
pause -1
