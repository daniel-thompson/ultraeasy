lib_LTLIBRARIES = libultraeasy.la
libultraeasy_la_SOURCES = ultraeasy.c ue_link.c util.c facade.c
libultraeasy_la_LDFLAGS = -version-info 0:0:0 -export-symbols-regex '^ultraeasy_'

pkginclude_HEADERS = ultraeasy.h

bin_PROGRAMS=ultraeasy
ultraeasy_SOURCES = main.c util.c
# Setting _CPPFLAGS avoids object file name conflicts between the library and
# the application (both of which use util.c)
ultraeasy_CPPFLAGS = -DOES_SOMETHING_MAGIC_TO_AUTOMAKE
ultraeasy_LDADD = libultraeasy.la
ultraeasy_DEPENDENCIES = libultraeasy.la
