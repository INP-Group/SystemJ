#!/bin/sh

MYNAME=$0

usage() {
	echo "Usage: $MYNAME -o <outfile.c> -s <clientsystem> <clientslist.lst>"
	echo "   or: $MYNAME                -p <clientsystem> <clientslist.lst>"
	exit 1
}

if   [ "$#" -eq 5  -a  "z$1" = "z-o"  -a  "z$3" = "z-s" ]
then
	OUTFILE=$2
	SYSTEM=$4
	LIST=$5
	ACTION="mkclient"
elif [ "$#" -eq 3  -a  "z$1" = "z-p" ]
then
	SYSTEM=$2
	LIST=$3
	ACTION="clientprog"
else
	usage
fi

LINE=`grep "^$SYSTEM|" $LIST`
if [ -z "$LINE" ]
then
	echo "$MYNAME: no system '$SYSTEM' found in $LIST"
	exit 2
fi

APP_NAME=` echo $LINE | awk -F'|' '{print $2}'`
APP_CLASS=`echo $LINE | awk -F'|' '{print $3}'`
APP_ICON=` echo $LINE | awk -F'|' '{print $4}'`
APP_TITLE=`echo $LINE | sed -e 's/^[^|]*|[^|]*|[^|]*|[^|]*|//'`

if [ -z "$APP_ICON" ]
then
	INCLUDE_ICON='/*No icon for #include*/'
	ICON_REF=NULL
else
	INCLUDE_ICON="#include \"pixmaps/${APP_ICON}.xpm\""
	ICON_REF=${APP_ICON}_xpm
fi

# Check for leading "*"
SUBSYSFILE="DB/db_${SYSTEM}.h"
PHYSINFOREF="${SYSTEM}_physinfo"
PHYSINFOCOUNT="countof(${SYSTEM}_physinfo)"
if   (echo "$APP_NAME" | grep '^[*]' >/dev/null)
then
  APP_NAME=` echo "$APP_NAME"|sed -e s/^[*]//`
  SUBSYSFILE="allsystems.h"
  PHYSINFOREF="(physprops_t *)physinfo_database"
  PHYSINFOCOUNT=-1
elif (echo "$APP_NAME" | grep '^[+]' >/dev/null)
then
  APP_NAME=` echo "$APP_NAME"|sed -e s/[+]//`
  PHYSINFOREF="(void *)${SYSTEM}_physinfo_database"
  PHYSINFOCOUNT=-1
fi

# Check for "@serverref"
DEFSERVER=""
CLIENTPROG=""
OPTIONS=""
if (echo "$APP_NAME" | grep '@' >/dev/null)
then
  DEFSERVER=`echo $APP_NAME | awk -F@ '{print $2}'`
  APP_NAME=` echo $APP_NAME | awk -F@ '{print $1}'`

  # Okay, let's also check for ";options="
  if (echo "$DEFSERVER" | grep ';' >/dev/null)
  then
    PARAMS=`   echo $DEFSERVER | sed -e 's/;/|/' | awk -F'|' '{print $2}'`
    DEFSERVER=`echo $DEFSERVER | awk -F';' '{print $1}'`

    for OPT in `echo $PARAMS | sed -e 's/;/ /g'`
    do
      ARGS=""
      if (echo "$OPT" | grep '=' >/dev/null)
      then
        ARGS=`echo $OPT | awk -F'=' '{print $2}'`
        OPT=` echo $OPT | awk -F'=' '{print $1}'`
      fi
      case "$OPT" in
        options)
          OPTIONS=$ARGS
          ;;
        clientprog)
          CLIENTPROG=$ARGS
          ;;
        *)
          echo "$MYNAME: unknown option '$OPT'" >&2
          exit 1
          ;;
      esac
    done
  fi
fi

if [ "$ACTION" = "clientprog" ]
then
	[ -z "$CLIENTPROG" ]  &&  CLIENTPROG=chlclient
	echo "$CLIENTPROG"
	exit 0
fi

(
cat <<EOT
#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "$SUBSYSFILE"

$INCLUDE_ICON

DEFINE_SUBSYS_DESCR($SYSTEM, "$DEFSERVER",
                    "$APP_NAME", "$APP_CLASS",
                    "$APP_TITLE", $ICON_REF,
                    ${PHYSINFOREF}, ${PHYSINFOCOUNT}, ${SYSTEM}_grouping,
                    "$CLIENTPROG", "$OPTIONS");
EOT
) > $OUTFILE
