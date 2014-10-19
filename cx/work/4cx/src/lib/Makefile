######################################################################
#                                                                    #
#  lib/Makefile                                                      #
#      Rules for making CX lib/ (libraries) subtree                  #
#                                                                    #
######################################################################

CSUBDIRS=	misc useful \
		cxlib cda datatree Cdr KnobsCore \
		srv rem v2cx
XSUBDIRS=	AuxMotifWidgets Xh MotifKnobs Chl
ifneq "$(NOX)" ""
  XSUBDIRS=	
endif
SUBDIRS=	$(CSUBDIRS) $(XSUBDIRS)

TOPDIR=		..
include $(TOPDIR)/TopRules.mk

cda:		v2cx

# #### END OF Makefile ###############################################
