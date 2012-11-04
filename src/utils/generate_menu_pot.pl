#! /usr/bin/perl -w

use warnings;
use strict;
no locale;

use File::Basename;
use Text::ParseWords;

my %context;
my $text;
my $line;
my $linenum=1;
my $key;
my $start=0;

if (!$ARGV[0])
{
	print "$0 <menu file>\n";
	exit 1;
}

open MENU, '<'.$ARGV[0] or die "$!\n";
while (defined ($line = <MENU>))
{
	if ( $start == 1)
	{
		if ($line =~ /.*?\)/)
		{
			$start = 0;
			$text .= $line;
			$text =~ s{\n}{ }g;
			chomp $text;
 			if ( $text =~ /MULTI.*\((.*?),(.*?),(.*?),(.*?),(.*?),(.*?)\)/ )
 			{
				my @arr = quotewords('\s+', 1, $4);
				foreach my $str (@arr)
				{
					print "\n// " . basename($ARGV[0]) . ": $linenum\n" . "$str"
				}
 			}
 			if ( $text =~ /MULTI\s*\((.*?),(.*?),(.*?),(.*?),(.*?)\)/ )
			{
				my @arr = quotewords('\s+', 1, $3);
				foreach my $str (@arr)
				{
					print "\n// " . basename($ARGV[0]) . ": $linenum\n" . "$str"
				}
			}
			$text = "";
		}
		else
		{
			$text .= $line;
		}
	}
	elsif ($line =~ /MULTI/)
	{
		$start = 1;
		$text = $line;
		if ($text =~ /MULTI.*\((.*?),(.*?),(.*?),(.*?),(.*?),(.*?)\)/)
		{
			my @arr = quotewords('\s+', 0, $1);
			foreach my $str (@arr)
			{
				print "\n// " . basename($ARGV[0]) . ": $linenum\n" . "$str"
			}
			$start = 0;
		}
		if ($text =~ /MULTI\s*\((.*?),(.*?),(.*?),(.*?),(.*?)\)/)
		{
			my @arr = quotewords('\s+', 0, $1);
			foreach my $str (@arr)
			{
				print "\n// " . basename($ARGV[0]) . ": $linenum\n" . "$str\n" . "a\n"
			}
			$start = 0;
		}
	}
	elsif ($line =~ /text .*(\".*?\")/)
	{
		print  "\n// " . basename($ARGV[0]) . ": $linenum\n" . "$1\n" . "a\n";
	}
	$linenum++;
}
close MENU;
