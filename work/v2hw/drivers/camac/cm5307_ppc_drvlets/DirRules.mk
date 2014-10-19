
# V2HDIR=="" => local, otherwise foreign
ifeq "$(V2HDIR)" ""
  V2HDIR=	$(PRJDIR)
  DIR_PATH=	.
else
  DIR_PATH=	$(V2HDIR)/drivers/camac/cm5307_ppc_drvlets
endif

# CAMAC
SRCDIR=		../src
CAMAC_PFX=	cm5307
include		$(DIR_PATH)/../src/ShadowRules.mk

CM5307_DEFINE_DRIVER_O=	$(DIR_PATH)/cm5307_DEFINE_DRIVER.o
CM5307_CAMAC_O=		$(DIR_PATH)/cm5307_camac.o

# ???
DRVLET_BINARIES=$(addsuffix .drvlet, \
		  $(addprefix $(CAMAC_PFX)_, $(CAMACDRIVERS)) \
		 )
DIR_DEPENDS=	$(DRVLET_BINARIES:.drvlet=_drv.d)
DIR_TARGETS=	$(DRVLET_BINARIES)

%.drvlet:	%_drv.o
		$(_O_BIN_LINE)

EXPORTSFILES=	$(DRVLET_BINARIES) $(MONO_C_FILES) $(UNCFILES)
EXPORTSDIR=	lib/server/drivers/cm5307_ppc_drvlets

DIR_DEFINES=	-DTRUE_DEFINE_DRIVER_H_FILE='"cm5307_DEFINE_DRIVER.h"'
DIR_INCLUDES=	-I$(DIR_PATH)

######################################################################
X_RULES_DIR=	$(V2HDIR)/x-build/ppc860-linux
include		$(X_RULES_DIR)/x_rules.mk
######################################################################

$(DRVLET_BINARIES):	$(CM5307_DEFINE_DRIVER_O) $(CM5307_CAMAC_O) \
			$(LIBPZFRAME_DRV) $(LIBREMDRVLET) \
			$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)
