
#
#
#

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = dist-bzip2 dist-zip

am__tar=$(AMTAR) -ch --posix -f - "$$tardir"
am__untar=$(AMTAR) -x --posix -f -

SUBDIRS = 	\
	src		\
	test

openlibraries_configdir = $(libdir)/ardome-ml-$(OL_MAJOR).$(OL_MINOR).$(OL_SUB)/include
openlibraries_config_DATA = openlibraries_global_config.hpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ardome_ml.pc

install-exec-hook:
	( cd $(libdir) && rm -f ardome-ml && ln -sf ardome-ml-$(OL_MAJOR).$(OL_MINOR).$(OL_SUB) ardome-ml )
	( cd $(prefix)/include && rm -f ardome-ml && ln -sf ardome-ml-$(OL_MAJOR).$(OL_MINOR).$(OL_SUB) ardome-ml )
