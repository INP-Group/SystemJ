
LOCAL_SHD_C_FILES=	karpov_pickup_station_functions.c
LOCAL_SHD_H_FILES=	karpov_pickup_station_functions.h
LOCAL_SHD_SYMLINKS=	$(LOCAL_SHD_C_FILES) $(LOCAL_SHD_H_FILES)
$(LOCAL_SHD_SYMLINKS):	%:	$(SRCDIR)/%
		$(SCRIPTSDIR)/ln-sf_safe.sh $< $@

$(DIRECTDRIVERSSOURCES:.c=.d) $(LOCAL_SHD_C_FILES:.c=.d):	$(LOCAL_SHD_H_FILES)
#LOCAL_DEPENDS+=		$(LOCAL_SHD_C_FILES:.c=.d)
#^^^ doesn't work because specified AFTER GeneralRules.mk inclusion

$(DRVLET_BINARIES):	$(LOCAL_SHD_C_FILES:.c=.o)

#
SHD_GNTDFILES+=	$(LOCAL_SHD_SYMLINKS)

#$(LOCAL_SHD_SYMLINKS):
#		$(SCRIPTSDIR)/ln-sf_safe.sh $< $@

SHD_LDFLAGS+=	-lm
