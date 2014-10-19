SIMPLECLIENTSLIST=	SimpleClients.lst
SIMPLECLIENTS:=	$(shell grep -v '^ *\#' $(SIMPLECLIENTSLIST) | grep '^[a-z][a-z_0-9]*[|]' | awk -F'|' '{print $$1}')
BIN_SYMLINKS=	$(SIMPLECLIENTS)
DBSOURCES=	$(SIMPLECLIENTS:=_db.c)
DBFILES=	$(DBSOURCES:.c=.so)
ALLSYSTEMS_H=	allsystems.h

MONO_C_FILES=	$(DBFILES)
UNCFILES=	$(BIN_SYMLINKS)

EXPORTSFILES=	$(BIN_SYMLINKS)
EXPORTSDIR=	bin

EXPORTSFILES2=  $(DBFILES)
EXPORTSDIR2=    lib/chlclients

# --------------------------------------------------------------------
DIR_CFLAGS=	-fPIC

######################################################################
include		$(TOPDIR)/TopRules.mk
######################################################################

DIR_INTMFILES=	$(DBSOURCES)
DIR_BCKPFILES=	DB/*\~
DIR_GNTDFILES=	$(ALLSYSTEMS_H)

# --------------------------------------------------------------------

$(SIMPLECLIENTS):	$(SIMPLECLIENTSLIST)
		N=`$(MKCLIENT) -p $@ $(SIMPLECLIENTSLIST)`; [ "$$N" == "-" ]  ||  $(SCRIPTSDIR)/ln-sf_safe.sh $$N $@

ifeq "$(OUTOFTREE)" ""
  MKCLIENT_PATH=	.
else
  MKCLIENT_PATH=	$(TOPDIR)/programs/chlclients
endif
MKCLIENT=	$(MKCLIENT_PATH)/mkclient.sh
MKPHYSDB=	$(MKCLIENT_PATH)/mkphysdb.sh

$(ALLSYSTEMS_H):	$(SIMPLECLIENTSLIST) $(MKPHYSDB)
		$(MKPHYSDB) -o $@ $(SIMPLECLIENTSLIST)

$(DBSOURCES):	$(SIMPLECLIENTSLIST) $(MKCLIENT) $(ALLSYSTEMS_H)
		$(MKCLIENT) -o $@ -s $(@:_db.c=) $(SIMPLECLIENTSLIST)
