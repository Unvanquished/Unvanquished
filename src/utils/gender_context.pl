#! /usr/bin/perl -w

no locale;
use strict;
use warnings;

my %context;
my $text;
my $line;
my $linenum=1;
my $key;

if (!$ARGV[0])
{
	print "$0 <source file with gender context>\n";
	exit 1;
}

open SOURCE, '<'.$ARGV[0] or die "$!\n";
while (defined ($line = <SOURCE>))
{
	if ($line =~ /G_\((.*?)\)/)
	{
		$context{ $linenum } = $1;
	}

	$linenum++;
}

close SOURCE;

print "\n";
foreach $key ( sort {$a <=> $b} keys %context )
{
	print "\#: " . $ARGV[0] . ":$key\n";
	print "msgctxt \"male\"\n";
	print "msgid $context{$key}\n";
	print "msgstr \"\"\n\n";

	print "\#: " . $ARGV[0] . ":$key\n";
	print "msgctxt \"female\"\n";
	print "msgid $context{$key}\n";
	print "msgstr \"\"\n\n";

	print "\#: " . $ARGV[0] . ":$key\n";
	print "msgctxt \"neuter\"\n";
	print "msgid $context{$key}\n";
	print "msgstr \"\"\n\n";
}
