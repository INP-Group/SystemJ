DEVS=		adc200me adc812me nadc200 nadc502 nadc333 nadc4 \
		c061621 \
		s2v \
		ottcam tsycamv \
		u0632
EXES=		$(DEVS) pzframeclient

DEVNNN_KNOBPLUGIN_FILES_F=	$(1)_data.o $(1)_gui.o $(1)_knobplugin.o
DEVNNN_FILES_F=		$(1).o $(1)_data.o $(1)_gui.o

# ADCs
adc200me_COMPONENTS=	$(call DEVNNN_FILES_F,adc200me)
adc812me_COMPONENTS=	$(call DEVNNN_FILES_F,adc812me)
nadc200_COMPONENTS=	$(call DEVNNN_FILES_F,nadc200)
nadc502_COMPONENTS=	$(call DEVNNN_FILES_F,nadc502)
nadc333_COMPONENTS=	$(call DEVNNN_FILES_F,nadc333)
nadc4_COMPONENTS=	$(call DEVNNN_FILES_F,nadc4)
c061621_COMPONENTS=	$(call DEVNNN_FILES_F,c061621)
s2v_COMPONENTS=		$(call DEVNNN_FILES_F,s2v)
# CCDs
ottcam_COMPONENTS=	$(call DEVNNN_FILES_F,ottcam)
tsycamv_COMPONENTS=	$(call DEVNNN_FILES_F,tsycamv)
# Misc
u0632_COMPONENTS=	$(call DEVNNN_FILES_F,u0632)

pzframeclient_COMPONENTS=	pzframeclient.o \
				$(foreach A, $(DEVS), \
				  $(call DEVNNN_KNOBPLUGIN_FILES_F, $A) \
				 )

UNCFILES=		$(foreach A, $(DEVS), \
			  $(call DEVNNN_KNOBPLUGIN_FILES_F, $A) \
			 )
DIR_DEPENDS=		$(UNCFILES:.o=.d)

LOCAL_LIBDEPS=	$(LIBPZFRAME_GUI) $(LIBPZFRAME_DATA)

# --------------------------------------------------------------------
PRJDIR=		..
SUBDIRS=
SECTION=	programs/xmclients/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
# --------------------------------------------------------------------

pzframeclient:	SPECIFIC_LIBS=$(LIBDL)
