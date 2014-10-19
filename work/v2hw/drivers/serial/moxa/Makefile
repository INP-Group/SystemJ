SRCDIR=		../src
232TYPE=	moxa
232DEVPFX=	M
include		$(SRCDIR)/ShadowRules.mk

MONO_C_FILES=	moxasetupkshd485 moxadumpserial
EXES=		cxv2moxaserver

cxv2moxaserver_COMPONENTS=	cxv2moxaserver.o cxv2moxaserver_drivers.o \
				$(DRIVERSOURCES:.c=.o)

EXPORTSFILES=	$(MONO_C_FILES) $(EXES)
EXPORTSDIR=	lib/server/drivers/moxa_arm_drvlets

LOCAL_DEFINES=	-DMOXA_KSHD485

######################################################################
PRJDIR=		../../..
X_RULES_DIR=	$(PRJDIR)/x-build/arm-linux

SECTION=	$(X_RULES_DIR)/x_rules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################


#
cxv2moxaserver_drivers.c:	Makefile
		(                                                     \
		  echo \#include'"remsrv.h"';                         \
		  $(foreach N, $(DRIVERSOURCES:_drv.c=),              \
		    echo 'extern CxsdDriverModRec $N_driver_rec;';)   \
		  echo 'static CxsdDriverModRec * drvtable[] = {';    \
		  $(foreach N, $(DRIVERSOURCES:_drv.c=),              \
		    echo '    &$N_driver_rec,';)                      \
		  echo '    NULL';                                    \
		  echo '};';                                          \
		  echo 'CxsdDriverModRec **remsrv_drivers = drvtable;'; \
		) >$@

LOCAL_GNTDFILES+=cxv2moxaserver_drivers.c

#
cxv2moxaserver:	$(LIBREMSRV) \
		$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)
$(232TYPE)setupkshd485 $(232TYPE)dumpserial:	$(LIBMISC)
