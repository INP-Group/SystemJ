######################################################################
#                                                                    #
#  programs/Makefile                                                 #
#      Rules for making CX programs/ subtree                         #
#                                                                    #
######################################################################

CSUBDIRS=	server drivers utils
XSUBDIRS=	tests
ifneq "$(NOX)" ""
  XSUBDIRS=	
endif
SUBDIRS=	$(CSUBDIRS) $(XSUBDIRS)

TOPDIR=		..
include $(TOPDIR)/TopRules.mk

# #### END OF Makefile ###############################################
