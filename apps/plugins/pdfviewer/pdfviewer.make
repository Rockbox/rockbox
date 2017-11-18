#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PDFVIEWERSRCDIR := $(APPSDIR)/plugins/pdfviewer
PDFVIEWERBUILDDIR := $(BUILDDIR)/apps/plugins/pdfviewer

ROCKS += $(PDFVIEWERBUILDDIR)/pdfviewer.rock

# -- taken from mupdf's makefile

FITZ_HDR := $(PDFVIEWERSRCDIR)/mupdf/include/$(PDFVIEWERSRCDIR)/mupdf/fitz.h $(wildcard $(PDFVIEWERSRCDIR)/mupdf/include/$(PDFVIEWERSRCDIR)/mupdf/fitz/*.h)
PDF_HDR := $(PDFVIEWERSRCDIR)/mupdf/include/$(PDFVIEWERSRCDIR)/mupdf/pdf.h $(wildcard $(PDFVIEWERSRCDIR)/mupdf/include/$(PDFVIEWERSRCDIR)/mupdf/pdf/*.h)
SVG_HDR := $(PDFVIEWERSRCDIR)/mupdf/include/$(PDFVIEWERSRCDIR)/mupdf/svg.h
HTML_HDR := $(PDFVIEWERSRCDIR)/mupdf/include/$(PDFVIEWERSRCDIR)/mupdf/html.h
THREAD_HDR := $(PDFVIEWERSRCDIR)/mupdf/include/$(PDFVIEWERSRCDIR)/mupdf/helpers/mu-threads.h

FITZ_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/fitz/*.c)
PDF_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/pdf/*.c)
XPS_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/xps/*.c)
SVG_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/svg/*.c)
CBZ_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/cbz/*.c)
HTML_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/html/*.c)
GPRF_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/gprf/*.c)
THREAD_SRC := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/helpers/mu-threads/*.c)

FITZ_SRC_HDR := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/fitz/*.h)
PDF_SRC_HDR := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/pdf/*.h) $(PDFVIEWERSRCDIR)/mupdf/source/pdf/pdf-name-table.h
XPS_SRC_HDR := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/xps/*.h)
SVG_SRC_HDR := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/svg/*.h)
HTML_SRC_HDR := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/html/*.h)
GPRF_SRC_HDR := $(wildcard $(PDFVIEWERSRCDIR)/mupdf/source/gprf/*.h)

OUT := $(PDFVIEWERBUILDDIR)/mupdf

FITZ_OBJ := $(FITZ_SRC:%.c=$(OUT)/%.o)
PDF_OBJ := $(PDF_SRC:%.c=$(OUT)/%.o)
XPS_OBJ := $(XPS_SRC:%.c=$(OUT)/%.o)
SVG_OBJ := $(SVG_SRC:%.c=$(OUT)/%.o)
CBZ_OBJ := $(CBZ_SRC:%.c=$(OUT)/%.o)
HTML_OBJ := $(HTML_SRC:%.c=$(OUT)/%.o)
GPRF_OBJ := $(GPRF_SRC:%.c=$(OUT)/%.o)
THREAD_OBJ := $(THREAD_SRC:%.c=$(OUT)/%.o)

$(FITZ_OBJ) : $(FITZ_HDR) $(FITZ_SRC_HDR)
$(PDF_OBJ) : $(FITZ_HDR) $(PDF_HDR) $(PDF_SRC_HDR)
$(XPS_OBJ) : $(FITZ_HDR) $(XPS_HDR) $(XPS_SRC_HDR)
$(SVG_OBJ) : $(FITZ_HDR) $(SVG_HDR) $(SVG_SRC_HDR)
$(CBZ_OBJ) : $(FITZ_HDR) $(CBZ_HDR) $(CBZ_SRC_HDR)
$(HTML_OBJ) : $(FITZ_HDR) $(HTML_HDR) $(HTML_SRC_HDR)
$(GPRF_OBJ) : $(FITZ_HDR) $(GPRF_HDR) $(GPRF_SRC_HDR)
$(THREAD_OBJ) : $(THREAD_HDR)

# --
# GNU Makefile for third party libraries used by MuPDF
#
# If thirdparty libraries are supplied, they will be built as
# static libraries.
#
# Use 'git submodule init' and 'git submodule update' to check
# out the thirdparty libraries from git.

FREETYPE_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/freetype
HARFBUZZ_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/harfbuzz
JBIG2DEC_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/jbig2dec
JPEGXR_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/jpegxr
LIBJPEG_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/libjpeg
LURATECH_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/luratech
MUJS_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/mujs
OPENJPEG_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/openjpeg/src/lib/openjp2
ZLIB_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/zlib

CURL_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/curl
GLFW_DIR := $(PDFVIEWERSRCDIR)/mupdf/thirdparty/glfw

CC_CMD = $(CC) $(CFLAGS) -c $< -o $@

# --- MuJS ---

ifneq "$(wildcard $(MUJS_DIR)/README)" ""

MUJS_OUT := $(OUT)/thirdparty/mujs
MUJS_SRC := one.c

MUJS_OBJ := $(addprefix $(MUJS_OUT)/, $(MUJS_SRC:%.c=%.o))

$(MUJS_OUT)/one.o: $(wildcard $(MUJS_DIR)/js*.c $(MUJS_DIR)/utf*.c $(MUJS_DIR)/regex.c $(MUJS_DIR)/*.h)

MKDIR = $(SILENT)mkdir

$(MUJS_OUT):
	$(MKDIR) -p $@
$(MUJS_OUT)/%.o: $(MUJS_DIR)/%.c | $(MUJS_OUT)
	$(CC)

MUJS_CFLAGS := -I$(MUJS_DIR)

else

MUJS_CFLAGS := -DFZ_ENABLE_JS=0

endif

# --- FreeType 2 ---

FREETYPE_OUT := $(OUT)/thirdparty/freetype
FREETYPE_SRC := \
	ftbase.c \
	ftbbox.c \
	ftbitmap.c \
	ftdebug.c \
	ftfntfmt.c \
	ftgasp.c \
	ftglyph.c \
	ftinit.c \
	ftstroke.c \
	ftsynth.c \
	ftsystem.c \
	fttype1.c \
	cff.c \
	psaux.c \
	pshinter.c \
	psnames.c \
	raster.c \
	sfnt.c \
	smooth.c \
	truetype.c \
	type1.c \
	type1cid.c \

FREETYPE_OBJ := $(addprefix $(FREETYPE_OUT)/, $(FREETYPE_SRC:%.c=%.o))

$(FREETYPE_OUT):
	$(MKDIR) -p $@

FT_CFLAGS := -DFT2_BUILD_LIBRARY -DDARWIN_NO_CARBON \
	'-DFT_CONFIG_MODULES_H="slimftmodules.h"' \
	'-DFT_CONFIG_OPTIONS_H="slimftoptions.h"'

FREETYPE_CFLAGS := -I$(PDFVIEWERSRCDIR)/mupdf/scripts/freetype -I$(FREETYPE_DIR)/include

$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/base/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/cff/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/cid/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/psaux/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/pshinter/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/psnames/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/raster/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/smooth/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/sfnt/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/truetype/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/type1/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)

# --- HarfBuzz ---

ifneq "$(wildcard $(HARFBUZZ_DIR)/README)" ""

HARFBUZZ_OUT := $(OUT)/thirdparty/harfbuzz
HARFBUZZ_SRC := \
	hb-blob.cc \
	hb-buffer.cc \
	hb-buffer-serialize.cc \
	hb-common.cc \
	hb-face.cc \
	hb-fallback-shape.cc \
	hb-font.cc \
	hb-ft.cc \
	hb-ot-font.cc \
	hb-ot-layout.cc \
	hb-ot-map.cc \
	hb-ot-shape-complex-arabic.cc \
	hb-ot-shape-complex-default.cc \
	hb-ot-shape-complex-hangul.cc \
	hb-ot-shape-complex-hebrew.cc \
	hb-ot-shape-complex-indic-table.cc \
	hb-ot-shape-complex-indic.cc \
	hb-ot-shape-complex-myanmar.cc \
	hb-ot-shape-complex-thai.cc \
	hb-ot-shape-complex-tibetan.cc \
	hb-ot-shape-complex-use-table.cc \
	hb-ot-shape-complex-use.cc \
	hb-ot-shape-fallback.cc \
	hb-ot-shape-normalize.cc \
	hb-ot-shape.cc \
	hb-ot-tag.cc \
	hb-set.cc \
	hb-shape-plan.cc \
	hb-shape.cc \
	hb-shaper.cc \
	hb-ucdn.cc \
	hb-unicode.cc \
	hb-warning.cc

#	hb-coretext.cc
#	hb-directwrite.cc
#	hb-glib.cc
#	hb-gobject-structs.cc
#	hb-graphite2.cc
#	hb-icu.cc
#	hb-uniscribe.cc

HARFBUZZ_OBJ := $(addprefix $(HARFBUZZ_OUT)/, $(HARFBUZZ_SRC:%.cc=%.o))

$(HARFBUZZ_OUT):
	$(MKDIR) -p $@
$(HARFBUZZ_OUT)/%.o: $(HARFBUZZ_DIR)/src/%.cc | $(HARFBUZZ_OUT)
	$(CC) -DHAVE_OT -DHAVE_UCDN -DHB_NO_MT $(FREETYPE_CFLAGS) \
		-Dhb_malloc_impl=hb_malloc -Dhb_calloc_impl=hb_calloc \
		-Dhb_free_impl=hb_free -Dhb_realloc_impl=hb_realloc \
		-fno-rtti -fno-exceptions -fvisibility-inlines-hidden --std=c++0x -c $< -o $@

HARFBUZZ_CFLAGS := -I$(HARFBUZZ_DIR)/src
else
HARFBUZZ_CFLAGS := $(SYS_HARFBUZZ_CFLAGS)
HARFBUZZ_LIBS := $(SYS_HARFBUZZ_LIBS)
endif

# --- LURATECH ---

ifneq "$(wildcard $(LURATECH_DIR)/ldf_jb2)$(wildcard $(LURATECH_DIR)/lwf_jp2)" ""

LURATECH_OUT := $(OUT)/thirdparty/luratech
LURATECH_SRC := \
	jb2_adt_cache.c \
	jb2_adt_decoder_halftone_region.c \
	jb2_adt_encoder_text_region.c \
	jb2_adt_huffman_tree.c \
	jb2_adt_symbol_instance.c \
	jb2_adt_context_ref_encoder.c \
	jb2_adt_context_ref_buffer.c \
	jb2_adt_context_ref_encoder.c \
	jb2_adt_mq_encoder.c \
	jb2_adt_decoder_text_region.c \
	jb2_adt_huffman_table_user_defined.c \
	jb2_adt_mmr_tables.c \
	jb2_adt_huffman_table_symbol.c \
	jb2_adt_component.c \
	jb2_adt_context_buffer.c \
	jb2_adt_context_decoder.c \
	jb2_adt_context_encoder.c \
	jb2_adt_context_ref_decoder.c \
	jb2_adt_decoder_collective_bitmap.c \
	jb2_adt_decoder_generic_region.c \
	jb2_adt_decoder_pattern_dict.c \
	jb2_adt_decoder_symbol_dict.c \
	jb2_adt_encoder_symbol_dict.c \
	jb2_adt_external_cache.c \
	jb2_adt_file.c \
	jb2_adt_file_extras.c \
	jb2_adt_handle_document.c \
	jb2_adt_huffman_decoder.c \
	jb2_adt_huffman_encoder.c \
	jb2_adt_huffman_table.c \
	jb2_adt_huffman_table_standard.c \
	jb2_adt_location.c \
	jb2_adt_memory.c \
	jb2_adt_message.c \
	jb2_adt_mmr_decoder.c \
	jb2_adt_mq_decoder.c \
	jb2_adt_mq_encoder.c \
	jb2_adt_mq_state.c \
	jb2_adt_pattern_dict.c \
	jb2_adt_pdf_file.c \
	jb2_adt_pdf_stream.c \
	jb2_adt_props_decompress.c \
	jb2_adt_read_bit_buffer.c \
	jb2_adt_read_data.c \
	jb2_adt_render_common.c \
	jb2_adt_render_generic_region.c \
	jb2_adt_render_halftone_region.c \
	jb2_adt_render_text_region.c \
	jb2_adt_run_array.c \
	jb2_adt_segment_array.c \
	jb2_adt_segment.c \
	jb2_adt_segment_end_of_stripe.c \
	jb2_adt_segment_generic_region.c \
	jb2_adt_segment_halftone_region.c \
	jb2_adt_segment_page_info.c \
	jb2_adt_segment_pattern_dict.c \
	jb2_adt_segment_region.c \
	jb2_adt_segment_symbol_dict.c \
	jb2_adt_segment_table.c \
	jb2_adt_segment_text_region.c \
	jb2_adt_segment_types.c \
	jb2_adt_stack.c \
	jb2_adt_symbol.c \
	jb2_adt_symbol_dict.c \
	jb2_adt_symbol_unify.c \
	jb2_adt_write_bits.c \
	jb2_adt_write_data.c \
	jb2_adt_write_pdf.c \
	jb2_common.c \
	jb2_license_dummy.c \
	jp2_adt_band_array.c \
	jp2_adt_band_buffer.c \
	jp2_adt_block_array.c \
	jp2_adt_cache.c \
	jp2_adt_comp.c \
	jp2_adt_component_array.c \
	jp2_adt_decomp.c \
	jp2_adt_ebcot_decoder.c \
	jp2_adt_external_cache.c \
	jp2_adt_image.c \
	jp2_adt_memory.c \
	jp2_adt_mq_decoder.c \
	jp2_adt_mq_state.c \
	jp2_adt_packet_decoder.c \
	jp2_adt_precinct_array.c \
	jp2_adt_rate.c \
	jp2_adt_rate_list.c \
	jp2_adt_read_bits.c \
	jp2_adt_read_data.c \
	jp2_adt_reader_requirements.c \
	jp2_adt_resolution_array.c \
	jp2_adt_tile_array.c \
	jp2_adt_tlm_marker_array.c \
	jp2_adt_write_data.c \
	jp2_buffer.c \
	jp2c_code_cb.c \
	jp2c_coder.c \
	jp2c_codestream.c \
	jp2c_file_format.c \
	jp2c_format.c \
	jp2c_memory.c \
	jp2_code_cb.c \
	jp2_common.c \
	jp2c_progression.c \
	jp2c_quant.c \
	jp2c_wavelet.c \
	jp2c_wavelet_lifting.c \
	jp2c_weights.c \
	jp2c_write.c \
	jp2d_codestream.c \
	jp2d_decoder.c \
	jp2d_file_format.c \
	jp2d_format.c \
	jp2d_image.c \
	jp2d_memory.c \
	jp2d_partial_decoding.c \
	jp2d_progression.c \
	jp2d_quant.c \
	jp2d_scale.c \
	jp2d_wavelet.c \
	jp2d_wavelet_lifting.c \
	jp2d_write.c \
	jp2_icc.c \
	jp2_license.c \
	jp2_packet.c \
	jp2_tag_tree.c

LURATECH_OBJ := $(addprefix $(LURATECH_OUT)/, $(LURATECH_SRC:%.c=%.o))

$(LURATECH_OUT):
	$(MKDIR) -p $@
$(LURATECH_OUT)/%.o: $(LURATECH_DIR)/ldf_jb2/source/common/%.c | $(LURATECH_OUT)
	$(CC) \
		-I$(LURATECH_DIR)/ldf_jb2/source/common \
		-DLINUX -c $< -o $@
$(LURATECH_OUT)/%.o: $(LURATECH_DIR)/ldf_jb2/source/compress/%.c | $(LURATECH_OUT)
	$(CC) \
		-I$(LURATECH_DIR)/ldf_jb2/source/common \
		-DLINUX -c $< -o $@
$(LURATECH_OUT)/%.o: $(LURATECH_DIR)/lwf_jp2/library/source/%.c | $(LURATECH_OUT)
	$(CC) \
		-I$(LURATECH_DIR)/ldf_jb2/source/common \
		-DLINUX -c $< -o $@

LURATECH_CFLAGS := \
-I$(LURATECH_DIR)/ldf_jb2/source/common \
	-I$(LURATECH_DIR)/ldf_jb2/source/libraries \
	-I$(LURATECH_DIR)/ldf_jb2/source/compress \
	-I$(LURATECH_DIR)/lwf_jp2/library/source \
	-DHAVE_LURATECH

else # --- LURATECH ---

# --- JBIG2DEC ---

ifneq "$(wildcard $(JBIG2DEC_DIR)/README)" ""

JBIG2DEC_OUT := $(OUT)/thirdparty/jbig2dec
JBIG2DEC_SRC := \
	jbig2.c \
	jbig2_arith.c \
	jbig2_arith_iaid.c \
	jbig2_arith_int.c \
	jbig2_generic.c \
	jbig2_halftone.c \
	jbig2_huffman.c \
	jbig2_image.c \
	jbig2_metadata.c \
	jbig2_mmr.c \
	jbig2_page.c \
	jbig2_refinement.c \
	jbig2_segment.c \
	jbig2_symbol_dict.c \
	jbig2_text.c

JBIG2DEC_OBJ := $(addprefix $(JBIG2DEC_OUT)/, $(JBIG2DEC_SRC:%.c=%.o))

$(JBIG2DEC_OUT):
	$(MKDIR) -p $@
$(JBIG2DEC_OUT)/%.o: $(JBIG2DEC_DIR)/%.c | $(JBIG2DEC_OUT)
	$(CC) -DHAVE_STDINT_H -DJBIG_EXTERNAL_MEMENTO_H=\"mupdf/memento.h\"

JBIG2DEC_CFLAGS := -I$(JBIG2DEC_DIR)
else
JBIG2DEC_CFLAGS := $(SYS_JBIG2DEC_CFLAGS)
JBIG2DEC_LIBS := $(SYS_JBIG2DEC_LIBS)
endif

# --- OpenJPEG ---

ifneq "$(wildcard $(OPENJPEG_DIR)/openjpeg.h)" ""

OPENJPEG_OUT := $(OUT)/thirdparty/openjpeg
OPENJPEG_SRC := \
	bio.c \
	cidx_manager.c \
	cio.c \
	dwt.c \
	event.c \
	function_list.c \
	image.c \
	invert.c \
	j2k.c \
	jp2.c \
	mct.c \
	mqc.c \
	openjpeg.c \
	phix_manager.c \
	pi.c \
	ppix_manager.c \
	raw.c \
	t1.c \
	t2.c \
	tcd.c \
	tgt.c \
	thix_manager.c \
	tpix_manager.c \
	thread.c

OPENJPEG_OBJ := $(addprefix $(OPENJPEG_OUT)/, $(OPENJPEG_SRC:%.c=%.o))

$(OPENJPEG_OUT):
	$(MKDIR) -p $@
$(OPENJPEG_OUT)/%.o: $(OPENJPEG_DIR)/%.c | $(OPENJPEG_OUT)
	$(CC) -DOPJ_STATIC -DOPJ_HAVE_STDINT_H -DOPJ_HAVE_INTTYPES_H -DUSE_JPIP

OPENJPEG_CFLAGS += -I$(OPENJPEG_DIR)
else
OPENJPEG_CFLAGS := $(SYS_OPENJPEG_CFLAGS)
OPENJPEG_LIBS := $(SYS_OPENJPEG_LIBS)
endif

endif # --- LURATECH ---

# --- JPEG library from IJG ---

ifneq "$(wildcard $(LIBJPEG_DIR)/README)" ""

LIBJPEG_OUT := $(OUT)/thirdparty/libjpeg
LIBJPEG_SRC := \
	jaricom.c \
	jcomapi.c \
	jdapimin.c \
	jdapistd.c \
	jdarith.c \
	jdatadst.c \
	jdatasrc.c \
	jdcoefct.c \
	jdcolor.c \
	jddctmgr.c \
	jdhuff.c \
	jdinput.c \
	jdmainct.c \
	jdmarker.c \
	jdmaster.c \
	jdmerge.c \
	jdpostct.c \
	jdsample.c \
	jdtrans.c \
	jerror.c \
	jfdctflt.c \
	jfdctfst.c \
	jfdctint.c \
	jidctflt.c \
	jidctfst.c \
	jidctint.c \
	jmemmgr.c \
	jquant1.c \
	jquant2.c \
	jutils.c \

LIBJPEG_OBJ := $(addprefix $(LIBJPEG_OUT)/, $(LIBJPEG_SRC:%.c=%.o))

$(LIBJPEG_OUT):
	$(MKDIR) -p $@
$(LIBJPEG_OUT)/%.o: $(LIBJPEG_DIR)/%.c | $(LIBJPEG_OUT)
	$(CC) -Dmain=xxxmain

LIBJPEG_CFLAGS := -Iscripts/libjpeg -I$(LIBJPEG_DIR)
else
LIBJPEG_CFLAGS := $(SYS_LIBJPEG_CFLAGS) -DSHARE_JPEG
LIBJPEG_LIBS := $(SYS_LIBJPEG_LIBS)
endif

# --- jpegxr ---

ifneq "$(wildcard $(JPEGXR_DIR)/T835E.pdf)" ""

JPEGXR_OUT := $(OUT)/thirdparty/jpegxr
JPEGXR_SRC := \
	algo.c \
	api.c \
	flags.c \
	init.c \
	io.c \
	cr_parse.c \
	jpegxr_pixelformat.c \
	r_parse.c \
	r_strip.c \
	r_tile_spatial.c \
	r_tile_frequency.c \
	x_strip.c

JPEGXR_OBJ := $(addprefix $(JPEGXR_OUT)/, $(JPEGXR_SRC:%.c=%.o))

$(JPEGXR_OUT):
	$(MKDIR) -p $@

$(JPEGXR_OUT)/%.o: $(JPEGXR_DIR)/Software/%.c | $(JPEGXR_OUT)
	$(CC) $(JPEGXR_CFLAGS)

JPEGXR_CFLAGS := \
	-I$(JPEGXR_DIR) \
	-I$(JPEGXR_DIR)/Software \
	-DHAVE_JPEGXR

endif

# --- ZLIB ---

ifneq "$(wildcard $(ZLIB_DIR)/README)" ""

ZLIB_OUT := $(OUT)/thirdparty/zlib
ZLIB_SRC := \
	adler32.c \
	compress.c \
	crc32.c \
	deflate.c \
	inffast.c \
	inflate.c \
	inftrees.c \
	trees.c \
	uncompr.c \
	zutil.c \
	gzlib.c \
	gzwrite.c \
	gzclose.c \
	gzread.c \

ZLIB_OBJ := $(addprefix $(ZLIB_OUT)/, $(ZLIB_SRC:%.c=%.o))

$(ZLIB_OUT):
	$(MKDIR) -p $@
$(ZLIB_OUT)/%.o: $(ZLIB_DIR)/%.c | $(ZLIB_OUT)
	$(CC) -Dverbose=-1 -DHAVE_UNISTD_H -DHAVE_STDARG_H

ZLIB_CFLAGS := -I$(ZLIB_DIR)
else
ZLIB_CFLAGS := $(SYS_ZLIB_CFLAGS)
ZLIB_LIBS := $(SYS_ZLIB_LIBS)
endif

THIRD_OBJ := $(FREETYPE_OBJ) $(HARFBUZZ_OBJ) $(JBIG2DEC_OBJ) $(LIBJPEG_OBJ) $(JPEGXR_OBJ) $(LURATECH_OBJ) $(MUJS_OBJ) $(OPENJPEG_OBJ) $(ZLIB_OBJ)

# --

PDFVIEWER_SRC := $(call preprocess, $(PDFVIEWERSRCDIR)/SOURCES) $(FITZ_SRC) $(FONT_SRC) $(PDF_SRC) $(XPS_SRC) $(SVG_SRC) $(CBZ_SRC) $(HTML_SRC) $(GPRF_SRC) $(THIRD_OBJ:%.o=%.c)
PDFVIEWER_OBJ := $(call c2obj, $(PDFVIEWER_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(PDFVIEWER_SRC)

PDFVIEWERFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2 -I$(PDFVIEWERSRCDIR)/mupdf/include -I$(PDFVIEWERSRCDIR)/mupdf/generated -I$(PDFVIEWERSRCDIR) $(FREETYPE_CFLAGS) $(FT_CFLAGS) $(HARFBUZZ_CFLAGS) $(JBIG2DEC_CFLAGS) $(LIBJPEG_CFLAGS) $(JPEGXR_CFLAGS) $(LURATECH_CFLAGS) $(MUJS_CFLAGS) $(OPENJPEG_CFLAGS) $(ZLIB_CFLAGS)

$(PDFVIEWERBUILDDIR)/pdfviewer.rock: $(PDFVIEWER_OBJ)

$(PDFVIEWERBUILDDIR)/%.o: $(PDFVIEWERSRCDIR)/%.c $(PDFVIEWERSRCDIR)/pdfviewer.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PDFVIEWERFLAGS) -c $< -o $@
