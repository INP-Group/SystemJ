HOSTS=		hemera ring1
hemera_SERVERS=	1 2 3 4 5
ring1_SERVERS=	35

BLKLISTS=	$(foreach H, $(HOSTS), \
		  $(foreach N, $($(H)_SERVERS), blklist-$H-$N.lst))
#STARTERCONFS=	$(foreach H, $(HOSTS), cx-starter-$H.conf) srvparams.conf

GIVEN_FILES=	$(BLKLISTS)

EXPORTSFILES=	$(GIVEN_FILES)
EXPORTSDIR=	configs

######################################################################
PRJDIR=		..
include		$(PRJDIR)/PrjRules.mk
######################################################################
