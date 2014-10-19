USRBINFILES=	cx-starter chlclient cdrclient
SYSBINFILES=	cx-run-agent runas

MONO_C_FILES=	$(USRBINFILES) $(SYSBINFILES)
UNCFILES=	cstart start-all-servers run-cx-server

EXPORTSFILES=	$(USRBINFILES) cstart start-all-servers
EXPORTSDIR=	bin

EXPORTSFILES2=	$(SYSBINFILES)
EXPORTSDIR2=	sbin


######################################################################
TOPDIR=		../..
include		$(TOPDIR)/TopRules.mk
######################################################################

cx-starter chlclient:	SPECIFIC_LIBS=$(LIBDL) $(MOTIF_LIBS)
cx-starter chlclient:	$(LIBCHL) $(LIBKNOBS) $(LIBCDR) $(LIBDATATREE) \
			$(LIBXH) $(LIBAUXMOTIFWIDGETS) \
			$(LIBCDA) $(LIBCX_ASYNC) \
			$(LIBUSEFUL) $(LIBXH_CXSCHEDULER) $(LIBMISC)

cdrclient:	$(LIBCDR) $(LIBDATATREE) \
		$(LIBCDA) $(LIBCX_ASYNC) \
		$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)
cdrclient:	SPECIFIC_LIBS=$(LIBDL) -lm

cstart:		cstart.sh
		cp -p $< $@  &&  chmod +x $@

run-cx-server:	run-cx-server.sh
		cp -p $< $@  &&  chmod +x $@

start-all-servers:	start-all-servers.sh
		cp -p $< $@  &&  chmod +x $@
