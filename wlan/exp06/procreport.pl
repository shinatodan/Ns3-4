#!/usr/bin/perl
# F.Qian, 2013
# analisys HWMP report

$infile=$ARGV[0];
#open (DATA,"<$infile") || die "Can't open $infile $!";
open (DATA,"<$infile") || die &Usage;

while($line = <DATA>) {
	if($line =~ /^<PeerLink/) {
		printf STDOUT "--------------------------------------------\n";
		for($i=0; $i<8; $i=$i+1) {
			$line = <DATA>;
			@x = split('=', $line);
			printf STDOUT "%20s \t %s",$x[0], $x[1];
		}
	}
}

# print usage and quit
sub Usage {
  	printf STDERR "Usage: procreport.pl <XML reportfile> \n";
  	exit(1);
}
