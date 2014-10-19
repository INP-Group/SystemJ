# CAN
SRCDIR=		../src
CANHAL_PFX=	skt
include		$(SRCDIR)/ShadowRules.mk

#
SOLIBS=			sktcankoz_drv.so
EXES=			sktcanmon flash_ubs flash_subord
sktcankoz_drv.so_COMPONENTS=	sktcankoz_drv.o cankoz_pre_lyr.o \
				$(CANDRIVERSSOURCES:.c=.o)
sktcanmon_COMPONENTS=		sktcanmon.o canmon_common.o

######################################################################
PRJDIR=         ../../..

SECTION=	programs/drivers/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

sktcanmon:	$(LIBMISC)

######################################################################
#!!! Should better be moved somewhere else -- liu/...?
flash_ubs.%:	SPECIFIC_DEFINES=-DCANHAL_FILE_H='"$(CANHAL_PFX)canhal.h"'
flash_ubs.d:	$(CANKOZ_DEFS) $(CANHAL_H)
flash_ubs:	$(LIBMISC)

flash_subord.%:	SPECIFIC_DEFINES=-DCANHAL_FILE_H='"$(CANHAL_PFX)canhal.h"'
flash_subord.d:	$(CANKOZ_DEFS) $(CANHAL_H)
flash_subord:	$(LIBMISC)
