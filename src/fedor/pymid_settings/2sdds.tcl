#!/bin/sh  
# \
exec tclsh "$0" "$@"

exec \
  plaindata2sdds linac_c_rw.txt linfieldrw.sdds -inputMode=ascii -noRowCount \
  -col=srcname,string \
  -col=midname,string 
