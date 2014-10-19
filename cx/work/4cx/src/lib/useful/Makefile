######################################################################
#                                                                    #
#  lib/useful/Makefile                                               #
#      Rules for making "useful" library                             #
#                                                                    #
######################################################################

ARLIBS=		libuseful.a libcxscheduler.a
libuseful.a_COMPONENTS=		fdiolib.o findfilein.o \
				cxldr.o \
				cxlogger.o \
				sendqlib.o \
				\
				ppf4td.o ppf4td_pipe.o \
				ppf4td_plaintext.o \
				ppf4td_m4.o ppf4td_cpp.o \
				ppf4td_mem.o
libcxscheduler.a_COMPONENTS=	cxscheduler.o

TOPDIR=		../..
include $(TOPDIR)/TopRules.mk

# #### END OF Makefile ###############################################
