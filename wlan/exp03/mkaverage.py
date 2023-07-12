#!/usr/bin/python
import sys
import argparse

parser = argparse.ArgumentParser(description='Process cmd line')
parser.add_argument('--fname', help='data file\'s name', dest='fname', required=True, action='store')
args = parser.parse_args()

w = 0.05

fd = open(args.fname)

line = fd.readline()
str = line.split()
avg = str[1]
str = line.split()
while line :
	str = line.split()
	new = str[1]
	avg = w*float(new) + (1.0-w)*float(avg)
	print str[0] ,"\t", avg
	line = fd.readline()
fd.close
