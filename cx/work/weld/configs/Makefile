HOSTS=		bike romeo
bike_SERVERS=	46
romeo_SERVERS=	45

BLKLISTS=	$(foreach H, $(HOSTS), \
		  $(foreach N, $($(H)_SERVERS), blklist-$H-$N.lst))
STARTERCONFS=	$(foreach H, $(HOSTS), cx-starter-$H.conf) srvparams.conf

GIVEN_FILES=	$(BLKLISTS) $(STARTERCONFS)

EXPORTSFILES=	$(GIVEN_FILES)
EXPORTSDIR=	configs

######################################################################
PRJDIR=		..
include		$(PRJDIR)/PrjRules.mk
######################################################################
