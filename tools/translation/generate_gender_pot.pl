#! /usr/bin/perl -w

# ===========================================================================
#
# Copyright (c) 2012 Unvanquished Developers
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# ===========================================================================

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
