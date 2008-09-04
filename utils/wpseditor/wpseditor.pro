TEMPLATE = subdirs
SUBDIRS = gui/src/QPropertyEditor gui libwps
libwps.commands = @$(MAKE) -C libwps
QMAKE_EXTRA_TARGETS += libwps
