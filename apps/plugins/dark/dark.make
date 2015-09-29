DARKSRCDIR := $(APPSDIR)/plugins/dark
DARKBUILDDIR := $(BUILDDIR)/apps/plugins/dark

ROCKS += $(DARKBUILDDIR)/gob_viewer.rock

GOB_VIEWER_SRC := $(call preprocess, $(DARKSRCDIR)/GOB_VIEWER_SOURCES)
GOB_VIEWER_OBJ := $(call c2obj, $(GOB_VIEWER_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(GOB_VIEWER_SRC)

$(DARKBUILDDIR)/gob_viewer.rock: $(GOB_VIEWER_OBJ)
