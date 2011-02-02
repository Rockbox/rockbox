#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

.SECONDEXPANSION: # $$(JAVA_OBJ) is not populated until after this
.SECONDEXPANSION: # $$(OBJ) is not populated until after this
.PHONY: apk classes clean dex dirs libs jar

$(BUILDDIR)/$(BINARY): $$(OBJ) $(VOICESPEEXLIB) $(FIRMLIB) $(SKINLIB)
	$(call PRINTS,LD $(BINARY))$(CC) -o $@ $^ $(LDOPTS) $(GLOBAL_LDOPTS)

PACKAGE=org.rockbox
PACKAGE_PATH=org/rockbox
ANDROID_DIR=$(ROOTDIR)/android
BINLIB_DIR=$(BUILDDIR)/libs/armeabi

java2class = $(addsuffix .class,$(basename $(subst $(ANDROID_DIR),$(BUILDDIR),$(1))))

# API version
ANDROID_PLATFORM_VERSION=8
ANDROID_PLATFORM=$(ANDROID_SDK_PATH)/platforms/android-$(ANDROID_PLATFORM_VERSION)

# android tools
AAPT=$(ANDROID_PLATFORM)/tools/aapt
DX=$(ANDROID_PLATFORM)/tools/dx
APKBUILDER=$(ANDROID_SDK_PATH)/tools/apkbuilder
ZIPALIGN=$(ANDROID_SDK_PATH)/tools/zipalign
KEYSTORE=$(HOME)/.android/debug.keystore
ADB=$(ANDROID_SDK_PATH)/platform-tools/adb

MANIFEST	:= $(ANDROID_DIR)/AndroidManifest.xml

R_JAVA		:= $(BUILDDIR)/gen/$(PACKAGE_PATH)/R.java
R_OBJ		:= $(BUILDDIR)/bin/$(PACKAGE_PATH)/R.class

JAVA_SRC	:= $(wildcard $(ANDROID_DIR)/src/$(PACKAGE_PATH)/Helper/*.java)
JAVA_SRC	+= $(wildcard $(ANDROID_DIR)/src/$(PACKAGE_PATH)/*.java)
JAVA_SRC	+= $(wildcard $(ANDROID_DIR)/src/$(PACKAGE_PATH)/widgets/*.java)
JAVA_OBJ	:= $(call java2class,$(subst $(ANDROID)/src/$(PACKAGE_PATH),$(ANDROID)/bin/$(PACKAGE_PATH),$(JAVA_SRC)))


LIBS		:= $(BINLIB_DIR)/$(BINARY) $(BINLIB_DIR)/libmisc.so
TEMP_APK	:= $(BUILDDIR)/bin/_rockbox.apk
TEMP_APK2	:= $(BUILDDIR)/bin/__rockbox.apk
DEX		:= $(BUILDDIR)/bin/classes.dex
JAR		:= $(BUILDDIR)/bin/classes.jar
AP_		:= $(BUILDDIR)/bin/resources.ap_
APK		:= $(BUILDDIR)/rockbox.apk

_DIRS		:= $(BUILDDIR)/___/$(PACKAGE_PATH)
DIRS		:= $(subst ___,bin,$(_DIRS))
DIRS		+= $(subst ___,gen,$(_DIRS))
DIRS		+= $(subst ___,data,$(_DIRS))
DIRS		+= $(BUILDDIR)/libs/armeabi

RES		:= $(wildcard $(ANDROID_DIR)/res/*/*)

CLEANOBJS += bin gen libs data

JAVAC_OPTS += -implicit:none -classpath $(ANDROID_PLATFORM)/android.jar:$(BUILDDIR)/bin

$(R_JAVA) $(AP_): $(MANIFEST) $(DIRS) $(RES)
	$(call PRINTS,AAPT $(subst $(BUILDDIR)/,,$@))$(AAPT) package -f -m \
		-J $(BUILDDIR)/gen -M $(MANIFEST) -S $(ANDROID_DIR)/res \
		-I $(ANDROID_PLATFORM)/android.jar -F $(AP_)

$(BUILDDIR)/bin/$(PACKAGE_PATH)/R.class: $(R_JAVA)
	$(call PRINTS,JAVAC $(subst $(ROOTDIR)/,,$<))javac -d $(BUILDDIR)/bin \
		$(JAVAC_OPTS) \
		-sourcepath $(ANDROID_DIR)/gen $<

$(BUILDDIR)/bin/$(PACKAGE_PATH)/%.class: $(ANDROID_DIR)/src/$(PACKAGE_PATH)/%.java $(BUILDDIR)/bin/$(PACKAGE_PATH)/R.class
	$(call PRINTS,JAVAC $(subst $(ROOTDIR)/,,$<))javac -d $(BUILDDIR)/bin \
		$(JAVAC_OPTS) \
		-sourcepath $(ANDROID_DIR)/src $<

$(JAR): $(JAVA_SRC) $(R_JAVA)
	$(call PRINTS,JAVAC $(subst $(ROOTDIR)/,,$?))javac -d $(BUILDDIR)/bin \
		$(JAVAC_OPTS) \
		-sourcepath $(ANDROID_DIR)/src:$(ANDROID_DIR)/gen $?
	$(call PRINTS,JAR $(subst $(BUILDDIR)/,,$@))jar cf $(JAR) -C $(BUILDDIR)/bin org

jar: $(JAR)

$(DEX): $(JAR)
	@echo "Checking for deleted class files" && $(foreach obj,$(JAVA_OBJ) $(R_OBJ), \
		(test -f $(obj) || (echo "$(obj) is missing. Run 'make classes' to fix." && false)) && ) true
	$(call PRINTS,DX $(subst $(BUILDDIR)/,,$@))$(DX) --dex --output=$@ $<

dex: $(DEX)

classes: $(R_OBJ) $(JAVA_OBJ)

$(BINLIB_DIR)/$(BINARY): $(BUILDDIR)/$(BINARY)
	$(call PRINTS,CP $(BINARY))cp $^ $@

$(BINLIB_DIR)/libmisc.so: $(BUILDDIR)/rockbox.zip
	$(call PRINTS,CP rockbox.zip)cp $^ $@

libs: $(LIBS)

$(TEMP_APK): $(AP_) $(DIRS) $(LIBS) $(DEX)
	$(call PRINTS,APK $(subst $(BUILDDIR)/,,$@))$(APKBUILDER) $@ \
	-u -z $(AP_) -f $(DEX) -nf $(BUILDDIR)/libs

$(KEYSTORE):
	$(call PRINTS,KEYTOOL debug.keystore)keytool -genkey \
		-alias androiddebugkey -keystore $@ \
		-storepass android -keypass android -validity 365 \
		-dname "CN=Android Debug,O=Android,C=US"

$(APK): $(TEMP_APK) $(BUILDDIR)/rockbox.zip $(KEYSTORE)
	$(SILENT)rm -f $@
	$(call PRINTS,SIGN $(subst $(BUILDDIR)/,,$@))jarsigner \
		-keystore "$(KEYSTORE)" -storepass "android" -keypass "android" \
		-signedjar $(TEMP_APK2) $(TEMP_APK) "androiddebugkey"
	$(SILENT)$(ZIPALIGN) -v 4 $(TEMP_APK2) $@ > /dev/null

$(DIRS):
	$(SILENT)mkdir -p $@

dirs: $(DIRS)

apk: $(APK)

install: apk
	$(ADB) install -r $(APK)

