# CAN
SRCDIR=		../src
CANHAL_PFX=	mar
include		$(SRCDIR)/ShadowRules.mk

#
SOLIBS=			marcankoz_drv.so
EXES=			marcanmon
marcankoz_drv.so_COMPONENTS=	marcankoz_drv.o cankoz_pre_lyr.o \
				$(CANDRIVERSSOURCES:.c=.o)
marcanmon_COMPONENTS=		marcanmon.o canmon_common.o

######################################################################
PRJDIR=         ../../..

SECTION=	programs/drivers/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

GIVEN_DIR=	$(PRJDIR)/given/marathon-2.4
LOCAL_INCLUDES+=-I$(GIVEN_DIR)

marcanmon:	$(LIBMISC)
