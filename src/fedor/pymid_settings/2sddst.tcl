#!/bin/sh  
# \
exec tclsh "$0" "$@"

exec \
  plaindata2sdds test_rw.txt test.sdds -inputMode=ascii -noRowCount \
  -col=srcname,string \
  -col=midname,string \
  -col=convert,string \
  -col=access,string \
  -col=smin,double \
  -col=smax,double \
  -col=s2m0,double \
  -col=s2m1,double \
  -col=m2s0,double \
  -col=m2s1,double