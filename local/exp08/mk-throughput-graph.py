#!/usr/bin/python
import sys

flist = ['5Mbps-5ms','5Mbps-10ms','5Mbps-15ms','5Mbps-20ms','5Mbps-25ms','5Mbps-30ms','5Mbps-35ms','5Mbps-40ms','5Mbps-45ms','5Mbps-50ms']

fdw = open("data/exp08.th", "w")
for fn in flist :
	fname = 'data/throughput-' + fn
	fd = open(fname)

	line = fd.readline()
	sum = 0.0
	cnt = 0
	while line :
		str = line.split()
		sum = sum + float(str[1])
	
		line = fd.readline()
		cnt = cnt+1
	fd.close

	size = len(fn)
	if size < 10 :
		n = fn[size-3:]
		n = n[0]
	else:
		n = fn[size-4:]
		n = n[:2]
		
	print n, "\t", sum/cnt, "\t", sum/cnt/(5*1024)
	format_strs = '%s \t %f \t %f\n' % (n, sum/cnt, sum/cnt/(5*1024))
	fdw.write(format_strs)
fdw.close

