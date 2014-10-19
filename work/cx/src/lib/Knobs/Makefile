ARLIBS=			libKnobs.a

Knobs_WIDGETS=		text alarmonoffled dial button selector choicebs \
			light slider sep label usertext
libKnobs.a_COMPONENTS=	Knobs_internals.o Knobs_public.o Knobs_simple.o \
			Knobs_widgetset.o $(Knobs_WIDGETS:%=Knobs_%_widget.o)

EXPORTSFILES=		$(ARLIBS)
EXPORTSDIR=		lib

######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################
