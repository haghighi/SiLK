#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowappend-append-cmd.pl 90396a06eb67 2015-08-05 22:05:43Z mthomas $")

use strict;
use SiLKTests;
use File::Temp ();


# set envvar to run app under valgrind when SK_TESTS_VALGRIND is set
check_silk_app('rwflowappend');

# find the apps we need.  this will exit 77 if they're not available
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');

# find the data files we use as sources, or exit 77
my %file;
$file{data} = get_data_or_exit77('data');

# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# create our tempdir
my $tmpdir = make_tempdir();

# the directory to hold the result of running the commands
my $cmd_dir = "$tmpdir/cmdout";
mkdir $cmd_dir
    or skip_test("Cannot create cmdout directory: $!");
# the name of the hourly file is written here
my $hourly_out = "$cmd_dir/hourly.txt";
my $hourly_format = "new hourly %s";


# create the two files
my %input_files = (
    dns  => File::Temp::mktemp("$tmpdir/in-S8_20090212.01.XXXXXX"),
    rest => File::Temp::mktemp("$tmpdir/in-S8_20090212.01.XXXXXX"),
    );

my $cmd = ("$rwfilter --type=in --sensor=S8 --pass=stdout"
           ." --stime=2009/02/12:01-2009/02/12:01 $file{data}"
           ." | $rwfilter --input-pipe=- --sport=53 --print-volume"
           ." --pass=$input_files{dns} --fail=$input_files{rest} 2>&1");
check_md5_output('5d44a50315bfe60379fc1b0e7fec5a04', $cmd);


# the command that wraps rwflowappend
$cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowappend-daemon.py",
                  ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                  ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                  "--copy $input_files{dns}:incoming",
                  "--copy $input_files{rest}:incoming",
                  "--basedir=$tmpdir",
                  "--",
                  "--polling-interval=5",
                  "--hour-file-command='echo $hourly_format >> $hourly_out'",
                  "--post-command='cp %s $cmd_dir/.'",
    );

# run it and check the MD5 hash of its output
check_md5_output('be50bfa0b38f0179132c2d2319ef1ad6', $cmd);


# the following directories should be empty
verify_empty_dirs($tmpdir, qw(error incoming));

# verify files are in the archive directory
verify_directory_files("$tmpdir/archive/in/2009/02/12", values %input_files);

# expected data file
my $data_file = "$tmpdir/root/in/2009/02/12/in-S8_20090212.01";
die "ERROR: Missing data file '$data_file'\n"
    unless -f $data_file;


# verify files are in the cmd_out directory
for my $k (keys %input_files) {
    my $f = $input_files{$k};
    $f =~ s,.*/,$cmd_dir/,;
    die "ERROR: Missing post-command file '$f'\n"
        unless -f $f;
}
if (! -f $hourly_out) {
    die "ERROR: Missing hour-file-command file '$hourly_out'\n";
}
else {
    open HOURLY, $hourly_out
        or die "ERROR: Cannot open '$hourly_out': $!\n";
    local $/;
    my $content = <HOURLY>;
    close HOURLY;
    die "ERROR: Bad hourly file '$hourly_out': undefined\n"
        unless defined $content;
    die "ERROR: Bad hourly file '$hourly_out' [$content] [", sprintf("$hourly_format\n", $data_file),"]\n"
        unless $content eq sprintf("$hourly_format\n", $data_file);
}


# compute MD5 of data file
my $data_md5;

$cmd = "$rwcat --compression=none --byte-order=little $data_file";
compute_md5(\$data_md5, $cmd);

# compute MD5 of the joining of the inputs files.  we don't know in
# which order things happened, so handle both cases.
my $input_keys = [sort keys %input_files];

for my $key_order ($input_keys, [reverse @$input_keys]) {
    my $input_md5;
    $cmd = join " ", ("$rwcat --compression=none --byte-order=little",
                      map {$input_files{$_}} @$key_order);
    compute_md5(\$input_md5, $cmd);
    if ($input_md5 eq $data_md5) {
        exit 0;
    }
}

die "ERROR: checksum mismatch [$data_md5] ($cmd)\n";
