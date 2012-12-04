#! /usr/bin/perl -w

use warnings;
use strict;
no locale;

use File::Basename;

if (!$ARGV[0])
{
	print "$0 <menu file>\n";
	exit 1;
}

open MENU, '<'.$ARGV[0] or die "$!\n";
my $code = do { local $/ = <MENU> };
close MENU;

my $filename = basename($ARGV[0]);

while ($code =~ /\s(?:text\s+(?<text>"[^"]*")|MULTIX?\s*\((?:(?:[^,]+),)?\s*(?<name>[^,]+)\s*,(?:[^,]+),\s*(?<choices>[^,]+)\s*,([^,)]+),([^)]+)\))/mg) {
	my $linenum;
	if (defined($+{'text'})) { # text "..."
		my $linenum = 1 + substr($code, 0, $+[1]) =~ y/\n//;
		printf("// %s:%i\n%s\na\n\n", $filename, $linenum, $+{'text'});
	}
	elsif (defined($+{'name'}) && defined($+{'choices'})) { # MULTI(...)
		# "name" arg
		$linenum = 1 + substr($code, 0, $+[2]) =~ y/\n//;
		printf("// %s:%i\n%s\na\n\n", $filename, $linenum, $+{'name'});

		# "choice" arg, consists of (string,value) pairs
		$linenum = 1 + substr($code, 0, $-[3]) =~ y/\n//;
		my $choices = $+{'choices'};
		while ($choices =~ /("[^"]*")\s*/g) {
			my $choice_linenum = $linenum + (substr($choices, 0, $+[1]) =~ y/\n//);
			printf("// %s:%i\n%s\na\n\n", $filename, $choice_linenum, $1);
		}
	}
}
