EXES=		ndbp cdmclient

ndbp_COMPONENTS=	ndbp.o \
			ndbp_image_elemplugin.o \
			ndbp_nadc200_elemplugin.o \
			ndbp_staroio.o
cdmclient_COMPONENTS=	cdmclient.o

LOCAL_OPT_LIBSCHEDULER=	$(LIBXH_CXSCHEDULER)
LOCAL_LIBS=		$(LIBDL)

# --------------------------------------------------------------------
PRJDIR=		..
SUBDIRS=
SECTION=	programs/xmclients/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
# --------------------------------------------------------------------
