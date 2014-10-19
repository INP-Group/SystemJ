EXES=		vacclient ippclient bpmclient kozclient \
		sukhphase \
		kshd485tool nkshd485tool
EXES+=sukhclient

vacclient_COMPONENTS=	vacclient.o vacclient_db.o vacclient_gui.o vacclient_meat.o
ippclient_COMPONENTS=	ippclient.o
bpmclient_COMPONENTS=	bpmclient.o \
			bpmclient_graph_knobplugin.o bpmclient_profile_knobplugin.o
sukhphase_COMPONENTS=	sukhphase.o
nsukhphase_COMPONENTS=	nsukhphase.o
sukhclient_COMPONENTS=	sukhclient.o
kozclient_COMPONENTS=	kozclient.o
kshd485tool_COMPONENTS=	kshd485tool.o

sukhclient_LIBDEPS=	$(LIBPZFRAME_GUI) $(LIBPZFRAME_DATA)


# --------------------------------------------------------------------
PRJDIR=		..
SUBDIRS=
SECTION=	programs/xmclients/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
# --------------------------------------------------------------------

kshd485tool.d kshd485tool.o:	SPECIFIC_INCLUDES=-I../chlclients/DB -I../../istc3682/include

PZDIR=		$(V2HDIR)/pzframes
sukhclient.d sukhclient.o:	SPECIFIC_INCLUDES=-I$(PZDIR)
sukhclient:			$(PZDIR)/nadc200_data.o \
				$(PZDIR)/nadc200_gui.o \
				$(PZDIR)/nadc200_knobplugin.o

$(EXES): SPECIFIC_LIBS=$(LIBDL)
