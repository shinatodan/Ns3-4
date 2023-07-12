#set terminal postscript eps
#set output 'Long-CongestionWindows.eps'
set xrange [5:50]
set yrange [0:100]
#set zrange [:]
#set isosamples 20,20
#set contour

set ztics 0,20000
set nozzeroaxis
set ticslevel 0

set key left
set grid
#set view 0,0,1,1

#nearint(x)=(x - floor(x) <= 0.5 ? floor(x) : floor(x)+1) 
#filter(x,y)=nearint(x/y)*y 

set xlabel 'delay(ms)'
set ylabel 'Times(s)'
set zlabel 'pkts'
set title 'Congestion Windows'

splot "data/cwnd-5Mbps-10ms" using 1:2:3  with lines lw 0.35 lc rgb "red" t 'cwnd-5M-10ms' ,\
      "data/cwnd-5Mbps-20ms" using 1:2:3  with lines lw 0.35 lc rgb "dark-green" t 'cwnd-5M-20ms' ,\
      "data/cwnd-5Mbps-30ms" using 1:2:3  with lines lw 0.35 lc rgb "blue" t 'cwnd-5M-30ms' ,\
      "data/cwnd-5Mbps-40ms" using 1:2:3  with lines lw 0.35 lc rgb "green" t 'cwnd-5M-40ms' ,\
      "data/cwnd-5Mbps-50ms" using 1:2:3  with lines lw 0.35 lc rgb "black" t 'cwnd-5M-50ms'
#     "" u (filter($1,6)):2 smooth csplines with linespoints pt 6 ps 1.0 t 'average'

#splot "data/cwnd-5Mbps-10ms" using 1:2:3  with lines lw 0.45 lc rgb "black" t 'cwnd-5M-10ms' ,\
#      "data/cwnd-5Mbps-20ms" using 1:2:3  with lines lw 0.45 lc rgb "black" t 'cwnd-5M-20ms' ,\
#      "data/cwnd-5Mbps-30ms" using 1:2:3  with lines lw 0.45 lc rgb "black" t 'cwnd-5M-30ms' ,\
#      "data/cwnd-5Mbps-40ms" using 1:2:3  with lines lw 0.45 lc rgb "black" t 'cwnd-5M-40ms' ,\
#      "data/cwnd-5Mbps-50ms" using 1:2:3  with lines lw 0.45 lc rgb "black" t 'cwnd-5M-50ms'
pause -1
