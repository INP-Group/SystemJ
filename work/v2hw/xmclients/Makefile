MONO_C_FILES=	tsycam

######################################################################
PRJDIR=		..
SECTION=	programs/xmclients/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

LIBTSYCAM=	$(PRJDIR)/lib/libtsycam.a
LIBMISCXUTILS=	$(PRJDIR)/lib/libmiscXutils.a
tsycam:		$(LIBTSYCAM) $(LIBMISCXUTILS)
# For Gridbox.h
tsycam.%:	SPECIFIC_INCLUDES=-I$(TOPDIR)/include
