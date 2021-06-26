#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

include $(ROOTDIR)/lib/microtar/microtar.make

.SECONDEXPANSION:

$(BUILDDIR)/spl.erosq: $(BUILDDIR)/spl.bin
	$(call PRINTS,MKSPL $(@F))$(TOOLSDIR)/mkspl-x1000 -type=nand -ppb=2 -bpp=2 $< $@

$(BUILDDIR)/bootloader.ucl: $(BUILDDIR)/bootloader.bin
	$(call PRINTS,UCLPACK $(@F))$(TOOLSDIR)/uclpack --nrv2e -9 $< $@ >/dev/null

.PHONY: $(BUILDDIR)/bootloader-info.txt
$(BUILDDIR)/bootloader-info.txt:
	$(call PRINTS,GEN $(@F))echo $(SVNVERSION) > $@

$(BUILDDIR)/$(BINARY): $(BUILDDIR)/spl.erosq \
					   $(BUILDDIR)/bootloader.ucl \
					   $(BUILDDIR)/bootloader-info.txt
	$(call PRINTS,TAR $(@F))tar -C $(BUILDDIR) \
		--numeric-owner --no-acls --no-xattrs --no-selinux \
		--mode=0644 --owner=0 --group=0 \
		-cf $@ $(call full_path_subst,$(BUILDDIR)/%,%,$^)
