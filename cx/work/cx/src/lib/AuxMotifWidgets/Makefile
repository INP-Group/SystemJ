ARLIBS=			libAuxMotifWidgets.a

libAuxMotifWidgets.a_COMPONENTS=	LiteClue.o XmLiteClue.o \
					Gridbox.o Table.o XmSepGridLayout.o \
					IncDecB.o InputOnly.o

EXPORTSFILES=		$(ARLIBS)
EXPORTSDIR=		lib

######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################

LiteClue.%:	SPECIFIC_DEFINES=-DNO_FONT_SET
Gridbox.%:	SPECIFIC_DEFINES=-DUSE_MOTIF
Table.%:	SPECIFIC_DEFINES=-DWINTERP # What a bugware!  It doesn't compile w/o this define because of declaring "extern strchr()", which is already a macro... (Damned AIX!)
