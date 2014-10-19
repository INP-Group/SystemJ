######################################################################
PRJDIR=		../..
SECTION=	Config.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

SUBDIRS=

ifeq ":$(OS):$(CPU_X86_COMPAT):" ":LINUX:YES:"
  MAY_BUILD_BIVME2=YES
else
  $(warning WARNING: Platform isn't LINUX-X86, so skipping build of bivme2)
endif

ifeq ":$(MAY_BUILD_BIVME2):" ":YES:"
  SUBDIRS+=        bivme2
endif

######################################################################
include		$(TOPDIR)/TopRules.mk
######################################################################

LOCAL_BCKPFILES=src/*~
