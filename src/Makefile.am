lib_LTLIBRARIES = libultraeasy.la
libultraeasy_la_SOURCES = ultraeasy.c ue_link.c util.c facade.c
libultraeasy_la_LDFLAGS = -version-info 0:0:0

pkginclude_HEADERS = ultraeasy.h

bin_PROGRAMS=ultraeasy
ultraeasy_SOURCES = main.c
ultraeasy_LDADD = libultraeasy.la
ultraeasy_DEPENDENCIES = libultraeasy.la
