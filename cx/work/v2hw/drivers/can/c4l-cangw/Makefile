# CAN
SRCDIR=		../src
CANHAL_PFX=	c4l
include		$(SRCDIR)/ShadowRules.mk

#
EXES=			canserver c4lcanmon
canserver_COMPONENTS=	canserver.o canserver_drivers.o cankoz_pre_lyr.o \
			$(CANDRIVERSSOURCES:.c=.o)
c4lcanmon_COMPONENTS=	c4lcanmon.o canmon_common.o

EXPORTSFILES=	$(EXES)
EXPORTSDIR=	lib/server/drivers/cangw_ppc_drvlets

######################################################################
PRJDIR=         ../../..
X_RULES_DIR=	$(PRJDIR)/x-build/ppc860-linux

SECTION=	$(X_RULES_DIR)/x_rules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

GIVEN_DIR=	$(PRJDIR)/given/cangw
LOCAL_INCLUDES+=-I$(GIVEN_DIR)

#
canserver_drivers.c:	$(LISTOFDRIVERS) Makefile
		(                                                     \
		  echo \#include'"remsrv.h"';                         \
		  $(foreach N, $(CANDRIVERS),                         \
		    echo 'extern CxsdDriverModRec $N_driver_rec;';)   \
		  echo 'static CxsdDriverModRec * drvtable[] = {'; \
		  $(foreach N, $(CANDRIVERS),                         \
		    echo '    &$N_driver_rec,';)              \
		  echo '    NULL';                                    \
		  echo '};';                                          \
		  echo 'CxsdDriverModRec **remsrv_drivers = drvtable;' \
		) >$@
LOCAL_GNTDFILES+=canserver_drivers.c

#
canserver:	$(LIBREMSRV) \
		$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)
c4lcanmon:	$(LIBMISC)
