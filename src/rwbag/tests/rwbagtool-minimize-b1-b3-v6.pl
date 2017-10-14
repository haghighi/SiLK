#! /usr/bin/perl -w
# MD5: a199fac876e27faf76115bc314d6a2b2
# TEST: ./rwbagtool --minimize ../../tests/bag1-v6.bag ../../tests/bag3-v6.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v6bag1} = get_data_or_exit77('v6bag1');
$file{v6bag3} = get_data_or_exit77('v6bag3');
check_features(qw(ipv6));
my $cmd = "$rwbagtool --minimize $file{v6bag1} $file{v6bag3} | $rwbagcat";
my $md5 = "a199fac876e27faf76115bc314d6a2b2";

check_md5_output($md5, $cmd);
