######################################################################
#                                                                    #
#  lib/rem/Makefile                                                  #
#      Rules for making remote-servers libraries                     #
#                                                                    #
######################################################################

ARLIBS=		libremdrvlet.a libremsrv.a
libremdrvlet.a_COMPONENTS=      remdrvlet_hostio.o \
				remdrvlet_listener.o \
				remdrvlet_srvmain.o \
				remdrvlet_utils.o
libremsrv.a_COMPONENTS=		remsrv_drvmgr.o remsrv_driver.o

TOPDIR=         ../..
include $(TOPDIR)/TopRules.mk

# #### END OF Makefile ###############################################
