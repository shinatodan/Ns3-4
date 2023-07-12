0) needed packages
   sudo apt-get install python-setuptools python-libxslt1 
   sudo easy_install lxml

1) use following command to make graph data
   % python mkhistgram.py jitterHistogram data/exp04-TCP-flowmon.xml

2) make graph
   % gnuplot plot-histgram.pl
