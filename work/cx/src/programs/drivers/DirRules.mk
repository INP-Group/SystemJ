include		$(TOPDIR)/Config.mk
# --------------------------------------------------------------------

# ==== Required tuning ===============================================

DIR_CFLAGS=	-fPIC

# ====

##MONO_C_FILES=	$(DRIVERSOURCES:.c=.so) \
##		$(UTILSSOURCES:.c=)

#!!! SOLIBS is for CAN-like drivers, which are composite and declared via SOLIBS instead of DRIVERSOURCES
EXPORTSFILES=	$(MONO_C_FILES) $(EXES) $(SOLIBS)
EXPORTSDIR=	lib/server/drivers

######################################################################
include $(TOPDIR)/TopRules.mk
######################################################################

# A temporary hack, till switch of all drivers/ to true GeneralRules-approach
##AUXDEPENDS=	$(SUBFLSSOURCES:.c=.d) $(UTILSSOURCES:.c=.d) $(LIBSSOURCES:.c=.d)
##ifneq "$(strip $(AUXDEPENDS))" ""
##  include $(AUXDEPENDS)
##endif
