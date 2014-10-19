######################################################################
#                                                                    #
#  include/Makefile                                                  #
#                                                                    #
#                                                                    #
######################################################################

LOCAL_BCKPFILES=	sysdeps/*\~
SUBDIRS=
TOPDIR=		..
include $(TOPDIR)/TopRules.mk


# ==== AuxMotifWidgets .h-files ======================================

AUXMOTIFHEADERS=	IncDecB.h \
			InputOnly.h \
			XmLiteClue.h \
			XmSepGridLayout.h

$(AUXMOTIFHEADERS):
			$(SCRIPTSDIR)/ln-sf_safe.sh ../lib/AuxMotifWidgets/$@ $@

files:			$(AUXMOTIFHEADERS)

CRITCLEANFILES=		$(AUXMOTIFHEADERS)

# #### END OF Makefile ###############################################
