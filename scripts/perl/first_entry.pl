#!/usr/bin/perl 
 
my $first_var = "2.6.32-358.23.2.ali1203.el5.x86_64";
my $first_var2 = $first_var;
$first_var2 =~ s/^.*-[0-9]*.[0-9]*.[0-9]*.//;
$first_var2 =~ s/.el[5-6].x86_64//;
print "Hello, World! $first_var $first_var2\n";
