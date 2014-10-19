######################################################################
#                                                                    #
#  programs/tests/Makefile                                           #
#      Rules for making CX test applications                         #
#                                                                    #
######################################################################

CHL_EXES=	tstknobs pult localtest
EXES=		tstknobs $(CHL_EXES)
tstknobs_COMPONENTS=	tstknobs.o
pult_COMPONENTS=	pult.o
localtest_COMPONENTS=	localtest

SUBDIRS=
TOPDIR=		../..
include $(TOPDIR)/TopRules.mk


# Damn, a hack for FC4@x86_64, where libXm.so is found in /usr/X11R6/lib64
ifeq "$(CPU)" "X86_64"
  MOTIF_LIBS_DIR=/usr/X11R6/lib64
else
  MOTIF_LIBS_DIR=/usr/X11R6/lib
endif
tstknobs:	SPECIFIC_LIBS=-L$(MOTIF_LIBS_DIR) \
		-lm -lXm -lXpm -lXt -lSM -lICE -lXext -lXmu -lX11
tstknobs:       $(LIBKNOBSCORE) $(LIBMOTIFKNOBS) \
		$(LIBCDR) $(LIBDATATREE) \
		$(LIBCDA) $(LIBCX_ASYNC) \
		$(LIBXH) $(LIBAUXMOTIFWIDGETS) \
		$(LIBUSEFUL) $(LIBXH_CXSCHEDULER) $(LIBMISC)
$(CHL_EXES):	SPECIFIC_LIBS=-L$(MOTIF_LIBS_DIR) \
		-lm -lXm -lXpm -lXt -lSM -lICE -lXext -lXmu -lX11
$(CHL_EXES):	$(LIBCHL) $(LIBKNOBSCORE) $(LIBMOTIFKNOBS) \
		$(LIBCDR) $(LIBDATATREE) \
		$(LIBCDA) $(LIBCX_ASYNC) \
		$(LIBXH) $(LIBAUXMOTIFWIDGETS) \
		$(LIBUSEFUL) $(LIBXH_CXSCHEDULER) $(LIBMISC)

# #### END OF Makefile ###############################################
