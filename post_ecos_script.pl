use FileHandle;

my $tmpfile = shift(@ARGV);
my $serstr = shift(@ARGV);
my $repstr = shift(@ARGV);

my $cmd = "mv $tmpfile $tmpfile.orig";

if ($ret = system("$cmd"))
{
   die("Can not execute  $cmd $! \n");
}

open TEMPFILEIN, "<$tmpfile.orig"
    || die "Can't read  input file \"$tmpfile.orig\". $!\n";

open TEMPFILEOUT, ">$tmpfile.new"
    || die "Can't write output file \"$tmpfile.new\". $!\n";

select TEMPFILEOUT;

my $found=0;
while (my $line = <TEMPFILEIN>)
{
   chomp $line;
   if ($line =~ s/$serstr/$repstr/)
   {
     $found = 1;
   }
   print "$line\n";
   next;
}

close(TEMPFILEIN);
close(TEMPFILEOUT);

my $cmd = "rm -f $tmpfile.orig";
if ($ret = system("$cmd"))
{
   die("Can not execute  $cmd $! \n");
}

my $cmd = "mv $tmpfile.new $tmpfile";
if ($ret = system("$cmd"))
{
   die("Can not execute  $cmd $! \n");
}
if("$found" == "0")
{
   die(" \"$serstr\" does not Found in file \"$tmpfile\" $! \n");
   exit 1;
}
