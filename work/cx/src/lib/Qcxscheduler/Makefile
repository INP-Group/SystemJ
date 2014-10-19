UNCFILES=	libQcxscheduler.a

EXPORTSFILES=	$(UNCFILES)
EXPORTSDIR=	lib

######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################

Q_PRO_FILE=     Qcxscheduler.pro
Q_MAKEFILE=	Qcxscheduler.mk

MAKEFILE_PARTS+=$(Q_PRO_FILE)
LOCAL_GNTDFILES=$(Q_MAKEFILE)

$(Q_MAKEFILE):	$(Q_PRO_FILE)
		qmake-qt4 -o $@ $< TOP_INCLUDES=$(TOPDIR)/include

CALL_QT_MAKE=	make -f $(Q_MAKEFILE)

libQcxscheduler.a:	$(Q_MAKEFILE) Qcxscheduler.cpp Qcxscheduler.h
		$(CALL_QT_MAKE)

.PHONY:		local-clean local-distclean
clean:		local-clean
distclean maintainer-clean crit-clean:	local-distclean

local-clean:	$(Q_MAKEFILE)
		$(CALL_QT_MAKE) clean
local-distclean:$(Q_MAKEFILE)
		$(CALL_QT_MAKE) distclean
