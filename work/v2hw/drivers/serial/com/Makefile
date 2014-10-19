SRCDIR=		../src
232TYPE=	com
232DEVPFX=	S
include		$(SRCDIR)/ShadowRules.mk

MONO_C_FILES=   $(DRIVERSOURCES:.c=.so) $(UTILSSOURCES:.c=)

######################################################################
PRJDIR=		../../..
SECTION=	programs/drivers/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

$(232TYPE)setupkshd485 $(232TYPE)dumpserial:	$(LIBMISC)
