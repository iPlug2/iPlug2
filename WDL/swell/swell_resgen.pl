#!/usr/bin/perl
use strict;
use warnings;
use Fcntl qw(:seek :flock);

my @input;
my %style_xlate = (
  "WS_CHILD" => "SWELL_DLG_WS_CHILD",
  "WS_THICKFRAME" => "SWELL_DLG_WS_RESIZABLE",
  "WS_EX_ACCEPTFILES" => "SWELL_DLG_WS_DROPTARGET",
  "WS_CLIPSIBLINGS" => "WS_CLIPSIBLINGS",
);

sub update_file {
  my($fn, $force, $data) = @_;

  open(FPC_FH, '+>>', $fn) or return $!;
  if (!flock(FPC_FH, LOCK_EX)) {
    close(FPC_FH);
    return "error locking file";
  }
  seek FPC_FH, 0, SEEK_SET;

  if ($force == 0)
  {
    my $v = "";
    while (<FPC_FH>) { $v .= $_; }
    if ($v eq $data) {
      close(FPC_FH);
      return "";
    }
    seek FPC_FH, 0, SEEK_SET;
  }
  truncate FPC_FH, 0;
  print FPC_FH $data;
  flock(FPC_FH, LOCK_UN);
  close(FPC_FH);
  return "OK";
}

sub convertquotes {
  return $_[0] unless $_[0] =~ /"/;
  my @a = split(/"/,$_[0], -1);
  my $ret = $a[0];
  my $qs = 0;
  for (my $x=1;$x<@a; $x++) {
    my $s = $a[$x];
    if ($s eq "" && $qs && $x+1 < @a) {
      $ret .= '\\"' . $a[++$x];
    } else {
      $ret .= "\"$s";
      $qs = !$qs;
    }
  }
  return $ret;
}

sub swell_rc2cpp_dialog
{
  my $errstr = "";
  my $retstr = '#ifndef SWELL_DLG_SCALE_AUTOGEN
#ifdef __APPLE__
  #define SWELL_DLG_SCALE_AUTOGEN 1.7
#else
  #define SWELL_DLG_SCALE_AUTOGEN 1.9
#endif
#endif
#ifndef SWELL_DLG_FLAGS_AUTOGEN
#define SWELL_DLG_FLAGS_AUTOGEN SWELL_DLG_WS_FLIPPED|SWELL_DLG_WS_NOAUTOSIZE
#endif

';

  my $dlg_state=0; # 1 = before BEGIN, 2=after BEGIN

  my $dlg_name="";
  my $dlg_size_w=0;
  my $dlg_size_h=0;
  my $dlg_title = "";
  my $dlg_styles = "SWELL_DLG_FLAGS_AUTOGEN";
  my $dlg_contents="";

  for (my $idx = 0; $idx < @input; $idx++)
  {
    my $x = $input[$idx];

    (my $y = $x) =~ s/^\s+|\s+$//g;
    if ($dlg_state>=2)
    {
      $y = "RTEXT" . $1 if $y =~ /^LTEXT(.*), *WS_EX_RIGHT$/;
      $dlg_contents .= $y . "\n";
      if ($y eq "END")
      {
        if ($dlg_state==2) { $dlg_styles.="|SWELL_DLG_WS_OPAQUE" };
        $retstr .= "#ifndef SET_${dlg_name}_SCALE\n" .
                   "#define SET_${dlg_name}_SCALE SWELL_DLG_SCALE_AUTOGEN\n" .
                   "#endif\n" .
                   "#ifndef SET_${dlg_name}_STYLE\n" .
                   "#define SET_${dlg_name}_STYLE $dlg_styles\n" .
                   "#endif\n" .
                   "SWELL_DEFINE_DIALOG_RESOURCE_BEGIN($dlg_name,SET_${dlg_name}_STYLE,\"$dlg_title\",$dlg_size_w,$dlg_size_h,SET_${dlg_name}_SCALE)\n";

        $dlg_contents =~ s/NOT\s+WS_VISIBLE/SWELL_NOT_WS_VISIBLE/gs;
        $retstr .= $dlg_contents;
        $retstr .= "SWELL_DEFINE_DIALOG_RESOURCE_END($dlg_name)\n\n\n";
        $dlg_state=0;
      }
      elsif (length($y)>1) { $dlg_state=3; }
    }
    else
    {
      my @parms = split " ", $y;
      if (@parms > 0)
      {
        if ($dlg_state == 0)
        {
          if (@parms>4 && ($parms[1] eq "DIALOGEX" || $parms[1] eq "DIALOG"))
          {
            $dlg_name=$parms[0];
            my $rdidx = 2;
            if ($parms[$rdidx] eq 'DISCARDABLE') { $rdidx++ };
            while ($rdidx < @parms && $parms[$rdidx] eq "") { $rdidx++; }
            $rdidx += 2;
            ($dlg_size_w = $parms[$rdidx++]) =~ s/,//g;
            ($dlg_size_h = $parms[$rdidx++]) =~ s/,//g;
            if (@parms >= $rdidx && $dlg_size_w ne "" && $dlg_size_h ne "")
            {
              $dlg_title="";
              $dlg_styles="SWELL_DLG_FLAGS_AUTOGEN";
              $dlg_contents="";
              $dlg_state=1;
            }
            else
            {
              $errstr .= "WARNING: corrupted $dlg_name resource\n";
            }
          }
        }
        elsif ($dlg_state == 1)
        {
          if ($parms[0] eq "BEGIN")
          {
            $dlg_state=2;
            $dlg_contents = $y ."\n";
          }
          else
          {
            if ($parms[0] eq "CAPTION")
            {
              $dlg_title = substr $y, 8;
              $dlg_title =~ s/^\s+|\s+$|"//g;
            }
            elsif ($parms[0] eq "STYLE" || $parms[0] eq "EXSTYLE")
            {
              my $rep=0;
              for (;($idx+1) < @input; $idx++)
              {
                my $next_line = $input[$idx+1];
                last unless $next_line =~ /^[ \t]/;

                $next_line =~ s/^\s+|\s+$//g;
                $y .= " " . $next_line;
                $rep++;
              }
              @parms = split " ", $y if $rep;
              my $opmode=0;
              for (my $rdidx = 1; $rdidx < @parms; $rdidx ++)
              {
                if ($parms[$rdidx] eq "NOT") { $opmode=1; }
                elsif ($opmode == 0)
                {
                  my $s = $style_xlate{$parms[$rdidx]};
                  $dlg_styles .= "|$s" if defined($s);
                }
                else { $opmode=0; }
              }
            }
          }
        }
      }
    }
  }
  $errstr .= "WARNING: there may have been a truncated  dialog resource ($dlg_name)\n" if $dlg_state != 0;

  $retstr .= "\n//EOF\n\n";
  return ( $retstr, $errstr );
}

sub swell_rc2cpp_menu
{
  my $retstr="";
  my $errstr="";

  my $menu_symbol="";
  my $menu_depth=0;
  for (@input)
  {
    my $x = $_;
    (my $y = $x) =~ s/^\s+|\s+$//g;

    if ($menu_symbol eq "")
    {
      my @parms = split " ", $y;
      if (@parms >= 2 && $parms[1] eq "MENU")
      {
        $menu_symbol = $parms[0];
        $menu_depth=0;
        $retstr .= "SWELL_DEFINE_MENU_RESOURCE_BEGIN($menu_symbol)\n";
      }
    }
    else
    {
      if ($y eq "END")
      {
        $menu_depth-=1;
        if ($menu_depth == 0)
        {
          $retstr .= "SWELL_DEFINE_MENU_RESOURCE_END($menu_symbol)\n\n\n";
        }
        $menu_symbol="" if $menu_depth < 1;
      }
      if ($menu_depth>0)
      {
        $x =~ s/, HELP\s*/\n/;
        $retstr .= $x;
      }
      $menu_depth+=1 if $y eq "BEGIN";
    }
  }

  $retstr .= "\n//EOF\n\n";

  return ($retstr, $errstr);
}

die("usage: swell_resgen.pl [--force] file.rc ...\n") if @ARGV<1;

my $forcemode = 0;
my $proc=0;
my $skipped=0;
my $err=0;
my $quiet=0;
for (my $x = 0; $x < @ARGV; $x ++)
{
   my $srcfn = $ARGV[$x];
   if ($srcfn eq "--quiet")
   {
     $quiet = 1;
     next;
   }
   if ($srcfn eq "--force")
   {
     $forcemode = 1;
     next;
   }
   if (!($srcfn =~ /[.]rc$/i))
   {
     $err++;
     print "$srcfn: does not end in .rc!\n";
     next;
   }

   if (!open(UTF_FH, "<", $srcfn))
   {
     $err++;
     print "$srcfn: could not open!\n";
     continue;
   }
   $_ = <UTF_FH>;
   if (/^\xff\xfe/ || /^\xfe\xff/) {
     @input = ( );
     my $order = (ord($_) == 0xff) ? 0 : 1;
     seek UTF_FH, 2, SEEK_SET;

     my $ret = "";
     while (read UTF_FH, my $s, 2)
     {
       $s = ($order == 1) ? (ord($s) | (ord(substr($s,1))<<8)) : (ord(substr($s,1)) | (ord($s)<<8));
       if ($s < 128) {
         $ret .= chr($s);
         if ($s == ord("\n")) {
           push(@input,convertquotes($ret));
           $ret = "";
         }
       }
       elsif ($s <= 0x7ff) {
         $ret .= chr(0xc0 | (($s>>6)&0x1f)) . chr(0x80 | ($s&0x3f));
       }
       else {
         $ret .= chr(0xe0 | (($s>>12)&0xf)) . chr(0x80 | (($s)>>6)&0x3f) . chr(0x80 | ($s&0x3f));
       }
     }
     push(@input,convertquotes($ret)) if $ret ne "";
   } else {
     $_ = substr($_,3) if /^\xef\xbb\xbf/;
     @input = ( $_ );
     while (<UTF_FH>) {
       push @input, convertquotes($_);
     }
   }

   close(UTF_FH);

   my $ofnmenu = $srcfn . "_mac_menu";
   my $ofndlg = $srcfn . "_mac_dlg";

   my ($dlg_str, $dlg_err) = swell_rc2cpp_dialog();
   my ($menu_str, $menu_err) = swell_rc2cpp_menu();

   if ($dlg_err ne "" || $menu_err ne "")
   {
     $err++;
     print "$srcfn: error";
     print " dialog: $dlg_err" if $dlg_err ne "";
     print " menu: $menu_err" if $menu_err ne "";
     print "\n";
     continue;
   }
   my $f="";
   my $ok = update_file($ofndlg, $forcemode, $dlg_str);
   $f .= ", dlg updated" if $ok eq "OK";
   if ($ok ne "OK" && $ok ne "") { print "error writing $ofndlg: $ok\n"; $err++; }

   $ok = update_file($ofnmenu, $forcemode, $menu_str);
   $f .= ", menu updated" if $ok eq "OK";
   if ($ok ne "OK" && $ok ne "") { print "error writing $ofnmenu: $ok\n"; $err++; }

   if ($f ne "")
   {
     $f =~ s/^, //;
     $proc++;
   }
   else
   {
     $skipped++;
     $f = "skipped";
   }

   print "$srcfn: $f\n" if $quiet == 0;
}

print "processed $proc, skipped $skipped, error $err\n" if $quiet==0 || $err>0;

