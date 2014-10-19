#!/usr/bin/perl -w

use strict;

$#ARGV != 2  &&  die "Usage: $0 DEVNAME ADC_COUNT OUT_COUNT\n";

my $devname = $ARGV[0];
my $adc_cnt = $ARGV[1];
my $out_cnt = $ARGV[2];

my $cn      = 0;
my $x;

my $au      = $adc_cnt > 0? "" : "_";
my $ou      = $out_cnt > 0? "" : "_";

print "#\ndevtype $devname w20i";
print ",w8i,r8i,w8i,w1i,r1i,w4i";
print ",r${adc_cnt}i"                           if $adc_cnt > 0;
print ",w${out_cnt}i,w${out_cnt}i,r${out_cnt}i" if $out_cnt > 0;
print ",w${out_cnt}i58"                         if $out_cnt > 0;
print " {\n";

print << "EOT"
	DO_RESET		0
	${au}ADC_MODE		1
	${au}ADC_BEG			2
	${au}ADC_END			3
	${au}ADC_TIMECODE		4
	${au}ADC_GAIN		5
	RESERVED6               6
	RESERVED7               7
	RESERVED8               8
	${ou}OUT_MODE		9
	${ou}DO_TAB_DROP		10
	${ou}DO_TAB_ACTIVATE		11
	${ou}DO_TAB_START		12
	${ou}DO_TAB_STOP		13
	${ou}DO_TAB_PAUSE		14
	${ou}DO_TAB_RESUME		15
	${ou}DO_CALIBRATE		16
	${ou}NUMCORR_MODE		17
	${ou}NUMCORR_V		18
	RESERVED19		19
EOT
;

$cn += 20;

print "\n";
for ($x = 0;  $x < 8;  $x++, $cn++) {print "\tOUTRB${x}\t\t\t${cn}\n";}
for ($x = 0;  $x < 8;  $x++, $cn++) {print "\tINPRB${x}\t\t\t${cn}\n";}
for ($x = 0;  $x < 8;  $x++, $cn++) {print "\tIR_IEB${x}\t\t\t${cn}\n";}
print "\tOUTR8B\t\t\t${cn}\n"; $cn++;
print "\tINPR8B\t\t\t${cn}\n"; $cn++;
print "\tIR_IE8B\t\t\t${cn}\n"; $cn++;
print "\tRESERVED_REGS_47\t${cn}\n"; $cn++;
print "\tRESERVED_REGS_48\t${cn}\n"; $cn++;
print "\tRESERVED_REGS_49\t${cn}\n"; $cn++;

if ($adc_cnt > 0)
{
    print "\n";
    for ($x = 0;  $x < $adc_cnt;  $x++, $cn++)
    {
        print "\tADC${x}\t\t\t${cn}\n";
    }
}

if ($out_cnt > 0)
{
    print "\n";
    for ($x = 0;  $x < $out_cnt;  $x++, $cn++)
    {
        print "\tOUT${x}\t\t\t${cn}\n";
    }

    print "\n";
    for ($x = 0;  $x < $out_cnt;  $x++, $cn++)
    {
        print "\tOUT_RATE${x}\t\t${cn}:OUT${x}\n";
    }

    print "\n";
    for ($x = 0;  $x < $out_cnt;  $x++, $cn++)
    {
        print "\tOUT_CUR${x}\t\t${cn}\n";
    }

    print "\n";
    for ($x = 0;  $x < $out_cnt;  $x++, $cn++)
    {
        print "\tTAB_OUT${x}\t\t${cn}\n";
    }
}
print "}\n";
