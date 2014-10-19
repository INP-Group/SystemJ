SOLIBS=				bigredcur.so
bigredcur.so_COMPONENTS=	bigredcur.o

ARLIBS=				libXh.a libXh_cxscheduler.a
libXh.a_COMPONENTS=		Xh_app.o Xh_window.o Xh_colors.o \
				Xh_toolbar.o Xh_statusline.o Xh_balloon.o \
				Xh_hilite.o \
				Xh_grid.o Xh_fdlg.o Xh_stddlg.o \
				Xh_viewport.o Xh_plot.o Xh_monoimg.o \
				Xh_utils.o Xh_globals.o
libXh_cxscheduler.a_COMPONENTS=	Xh_cxscheduler.o

EXPORTSFILES=		$(ARLIBS) $(SOLIBS)
EXPORTSDIR=		lib

######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################

Xh_grid.%:	SPECIFIC_DEFINES=-DUSE_WIDGET=SGL

bigredcur.o:	SPECIFIC_CFLAGS=-fPIC
