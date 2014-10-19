TOPDIR=		$(HOME)/work/cx/src
V2HDIR=		$(HOME)/work/v2hw

PRJ_INCLUDES=	-I$(PRJDIR)/include -I$(V2HDIR)/include

ifeq "$(strip $(SECTION))" ""
  SECTION=	TopRules.mk
endif

ifeq "$(filter-out .% /%,$(SECTION))" ""
  include	$(SECTION)
else
  include	$(TOPDIR)/$(SECTION)
endif

# --------------------------------------------------------------------

QULTDIR=	$(HOME)/work/qult
PRJ_INCLUDES+=	-I$(QULTDIR)/include
