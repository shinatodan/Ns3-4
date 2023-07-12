#!/usr/bin/python
from lxml import etree
import sys

def get_entry(trace, filename) :
    xml = etree.fromstring(open(filename, 'r').read())
    entries = xml.xpath('//'+trace)
    #print len(entries)

    for i in range(len(entries)) :
        if(len(entries[i]) < 2) :
            continue

        out = 'data/' + trace + '-' + str(i) + '.tr'
        fd = open(out, 'w')

        msgs = 'flow' + str(i+1) + ": entries " + str(len(entries[i]))
        #print msgs
        for e in entries[i] :
            #print "%s \t  %s" % (e.attrib['start'], e.attrib['count'])
            strs =  e.attrib['start'] +'\t' + e.attrib['count'] + '\n'
            fd.write(strs)

        fd.close()

if __name__ == '__main__':
    argvs = sys.argv
    argc  = len(argvs)

    if (argc != 3) :
        print 'Usage: python %s <trace_flag> <tracefile.xml>' % argvs[0]
        print '       trace_flag: jitterHistogram or delayHistogram'
        quit()

    get_entry(argvs[1], argvs[2])

