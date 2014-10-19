#!/bin/sh

MYNAME=$0
OUTFILE=$2
LIST=$3

usage() {
	echo "Usage: $MYNAME -o <outfile.h> <clientslist.lst>"
	exit 1
}

if [ "$#" -ne 3  -o  "z$1" != "z-o" ]
then
	usage
fi

FILTER='^[a-z][a-z_0-9]*[|][^|@]*@'

(
  echo "#include \"cda.h\""
  echo "/* Include all subsystems */"
  grep "$FILTER" "$LIST" | awk -F'|' '{print "#include \"DB/db_" $1 ".h\""}'

  echo "/* Physinfo database */"
  echo "static physinfodb_rec_t physinfo_database[] ="
  echo "{"
  grep "$FILTER" "$LIST" | awk -F'[|@]' '{split ($3, params, ";"); print "    {\"" params[1] "\", " $1 "_physinfo, countof(" $1 "_physinfo)},"}'
  echo "    {NULL, NULL, 0}"
  echo "};"
) > $OUTFILE
