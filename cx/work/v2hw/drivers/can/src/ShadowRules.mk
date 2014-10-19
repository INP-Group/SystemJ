.PHONY:		firsttarget
firsttarget:	all

LISTOFDRIVERS=	$(SRCDIR)/ListOfCanDrivers.mk
MAKEFILE_PARTS+=$(LISTOFDRIVERS) $(SRCDIR)/ShadowRules.mk
include		$(LISTOFDRIVERS)

CANDRIVERSSOURCES=$(addsuffix _drv.c, $(CANDRIVERS))

CANKOZ_CH=	cankoz_pre_lyr.c cankoz_pre_lyr.h
CANKOZ_DEFS=	cankoz_numbers.h cankoz_strdb.h canhal.h
CANUTILSSOURCES=canmon_common.c

CANSYMLINKS=	$(CANDRIVERSSOURCES) $(CANDRIVERSHEADERS) $(CANKOZ_CH) \
		$(CANUTILSSOURCES) $(CANKOZ_DEFS)
$(CANSYMLINKS):		%:	$(SRCDIR)/%
$(CANDRIVERSSOURCES:.c=.d) cankoz_pre_lyr.d:	cankoz_pre_lyr.h $(CANKOZ_DEFS)
cankoz_pre_lyr.%:		SPECIFIC_DEFINES=-DCANHAL_FILE_H='"$(CANHAL_PFX)canhal.h"'

$(CANUTILSSOURCES:.c=.d):	$(CANKOZ_DEFS)
canmon_common.%:		SPECIFIC_DEFINES=-DCANHAL_FILE_H='"$(CANHAL_PFX)canhal.h"'
vsdc2_drv.d:			vsdc2_defs.h

#
CANMAIN_EMPTYFILES=$(CANHAL_PFX)cankoz_drv.c $(CANHAL_PFX)canmon.c
$(CANMAIN_EMPTYFILES):
		touch $@

# General symlinks' management
SHD_SYMLINKS=	$(CANSYMLINKS)
SHD_GNTDFILES=	$(CANMAIN_EMPTYFILES) $(SHD_SYMLINKS)

$(SHD_SYMLINKS):
		$(SCRIPTSDIR)/ln-sf_safe.sh $< $@
