#!/usr/bin/python
import sys

argvs = sys.argv
argc  = len(argvs)

fd0 = open("data/exp07-n0.cwnd")
fd1 = open("data/exp07-n1.cwnd")
fd2 = open("data/exp07-n2.cwnd")
fdw = open("data/exp07-avg.cwnd", 'w')

line0 = fd0.readline()
line1 = fd1.readline()
line2 = fd2.readline()

while line0 :
	str0 = line0.split()
	str1 = line1.split()
	str2 = line2.split()

	#print str0[0] ,"\t", (float(str0[1])+float(str1[1])+float(str2[1]))/3.0
	strs = str0[0] + '\t' + str((float(str0[1])+float(str1[1])+float(str2[1]))/3.0) + '\n'
        fdw.write(strs)

	line0 = fd0.readline()
	line1 = fd1.readline()
	line2 = fd2.readline()
	if line0 == "" or line1 == "" or line2 == "" :
                break
fd0.close
fd1.close
fd2.close
fdw.close

print "The result is saved into data/exp07-avg.cwnd\n"
