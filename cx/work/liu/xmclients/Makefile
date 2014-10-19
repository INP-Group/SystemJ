EXES=		tantalclient liuclient

tantalclient_COMPONENTS=tantalclient.o

liuclient_COMPONENTS=	liuclient.o \
			key_offon_knobplugin.o \
			manyadcs_knobplugin.o \
			one200me_calc_knobplugin.o \
			two812ch_knobplugin.o \
			beamprof_knobplugin.o \
			avgdiff_knobplugin.o

PZDIR=		$(V2HDIR)/pzframes
LOCAL_INCLUDES= -I$(PZDIR)
LOCAL_LIBDEPS=	$(LIBPZFRAME_GUI) $(LIBPZFRAME_DATA)

# --------------------------------------------------------------------
PRJDIR=		..
SUBDIRS=
SECTION=	programs/xmclients/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
# --------------------------------------------------------------------

ADCNNN_KNOBPLUGIN_FILES_F=	$(PZDIR)/$(strip $(1))_data.o \
				$(PZDIR)/$(strip $(1))_gui.o \
				$(PZDIR)/$(strip $(1))_knobplugin.o

liuclient:	$(call ADCNNN_KNOBPLUGIN_FILES_F, adc200me) \
		$(call ADCNNN_KNOBPLUGIN_FILES_F, adc812me)

liuclient tantalclient:	SPECIFIC_LIBS=$(LIBDL)
