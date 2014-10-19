MONO_C_FILES=	dl200me_drv.so adc200me_drv.so adc812me_drv.so \
		pisoenc600_drv.so
LOCAL_DEPENDS=	pci4624.d
LOCAL_INCLUDES=-I$(PRJDIR)/../uspci/include

######################################################################
PRJDIR=		../..
SECTION=	programs/drivers/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

$(MONO_C_FILES):	pci4624.o $(LIBPZFRAME_DRV)
