SUBDIRS=	drv_i pixmaps sysdeps

EXPORTSFILES=	misc_types.h misclib.h misc_sepchars.h misc_macros.h fps_counter.h \
		fix_arpa_inet.h \
		cx_module.h cx_version.h \
		paramstr_parser.h fdiolib.h cxscheduler.h \
		cxnetacl.h cxlogger.h \
		cxldr.h \
		findfilein.h seqexecauto.h sendqlib.h lightscript_engine.h \
		memcasecmp.h timeval_utils.h \
		cxsd_driver.h \
		cx_sysdeps.h \
		cx.h cxlib.h \
		remdrv_proto.h remsrv.h remdrvlet.h remcxsd.h \
		cxdata.h cda.h \
		common_elem_macros.h \
		Xh.h Xh_types.h Xh_typesP.h Xh_fallbacks.h Xh_globals.h \
		Xh_viewport.h Xh_plot.h plotdata.h \
                Xh_monoimg.h \
		Cdr.h fqkn.h Cdr_script.h datatree.h \
		Chl.h ChlI.h \
		Knobs.h Knobs_types.h Knobs_typesP.h KnobsI.h \
		\
		pzframe_data.h  pzframe_gui.h  pzframe_knobplugin.h  pzframe_main.h \
		pzframe_cpanel_decor.h \
		fastadc_data.h  fastadc_gui.h  fastadc_knobplugin.h  fastadc_main.h \
		fastadc_cpanel_decor.h \
		vcamimg_data.h  vcamimg_gui.h  vcamimg_knobplugin.h  vcamimg_main.h \
		vcamimg_cpanel_decor.h \
		\
		pzframe_drv.h vdev.h
EXPORTSDIR=	include

MOTIFHEADERS=	LiteClue.h XmLiteClue.h \
		Gridbox.h Table.h XmSepGridLayout.h \
		IncDecB.h InputOnly.h

CRITCLEANFILES=	$(MOTIFHEADERS)

GIVEN_FILES=	$(MOTIFHEADERS)

######################################################################
TOPDIR=	        ..
include		$(TOPDIR)/TopRules.mk
######################################################################

$(MOTIFHEADERS):
		$(SCRIPTSDIR)/ln-sf_safe.sh ../lib/AuxMotifWidgets/$@ $@
