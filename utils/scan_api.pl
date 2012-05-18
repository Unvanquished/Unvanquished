#! /usr/bin/perl

no locale;

my %syscallfuncs, %syscalls;
my $func;
my $line;

if ($#ARGV < 1)
{
  print "$0 API_implementation.c API_header.h\n";
  exit 1;
}

open API, '<'.$ARGV[0] or die "$!\n";
while (defined ($line = <API>))
{
  if ($line =~ /[[:space:]](trap_[[:alnum:]_]+)[[:space:]]*\(.*\)$/)
  {
    $func = $1;
  }
  elsif ($line =~ /^}/)
  {
    undef $func;
  }
  elsif ($line =~ /[[:space:]]syscall[[:space:]]*\([[:space:]]*([[:upper:]][[:upper:][:digit:]_]+)/)
  {
    push @{$syscallfuncs{$1}}, $func if defined $func;
    undef $func;
  }
  elsif ($line =~ /^\/\/[[:space:]]*([[:upper:]][[:upper:][:digit:]_]+)[[:space:]]*=[[:space:]]*([[:alpha:]_][[:alnum:]_]+)/)
  {
    push @{$syscallfuncs{$1}}, $2;
  }
}
close API;

my $f;

for ($f = 1; $f <= $#ARGV; ++$f)
{
  open ENUM, '-|', 'cpp -C -DUSE_REFENTITY_ANIMATIONSYSTEM '.$ARGV[$f] or die $!;
  my $state = 0;
  my $value = 0;
  while (defined ($line = <ENUM>))
  {
    if ($state == 0)
    {
      $state = 1 if $line =~ /Import_s[[:space:]]*$/;
      next;
    }
    elsif ($state = 1)
    {
      if ($line =~ /^[[:space:]]+([[:upper:]][[:upper:][:digit:]_]+)[[:space:]]*=[[:space:]]*([[:digit:]]+)[[:space:]]*,?[[:space:]]*(?:\/\/|$)/)
      {
        my $enum = $1;
        $value = $2 + 0;
        $syscalls{$enum} = $value;
        ++$value;
        if ($line =~ /\/\/[[:space:]]*=[[:space:]]*([[:alpha:]][[:alnum:]_]+)$/)
        {
          push @{$syscallfuncs{$enum}}, $1;
        }
      }
      elsif ($line =~ /^[[:space:]]+([[:upper:]][[:upper:][:digit:]_]+)[[:space:]]*,?[[:space:]]*(?:\/\/|$)/)
      {
        my $enum = $1;
        $syscalls{$enum} = $value;
        ++$value;
        if ($line =~ /\/\/[[:space:]]*=[[:space:]]*([[:alpha:]][[:alnum:]_]+)$/)
        {
          push @{$syscallfuncs{$enum}}, $1;
        }
      }
      last if $line =~ /}/;
    }
  }
  close ENUM;
}

my %output;
my %check;
my @errs;

for $line (keys %syscallfuncs)
{
  for $func (@{$syscallfuncs{$line}})
  {
    push @errs, $func if defined $check{$func} && $check{$func} == 1;
    push @{$output{$syscalls{$line}}}, $func;
    ++$check{$func};
  }
}

die 'Multiple definitions of '.join (', ', sort @errs)."\n" if $#errs >= 0;

print "code\n\n";

for $line (sort { $a <=> $b } keys %output)
{
  for $func (@{$output{$line}})
  {
    printf "equ %-37s %d\n", $func, ~$line;
  }
}
