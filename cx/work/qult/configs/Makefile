HOSTS=		linac1 linac2 linac3 ring1 v5c-kuzn1
linac1_SERVERS=	1 2 3 5 10 30 31 33 34 36 50 57 59
linac2_SERVERS=
ring1_SERVERS=	4 32 35 56
v5c-kuzn1_SERVERS=	33

BLKLISTS=	$(foreach N, 59, blklist-$N.lst)
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
