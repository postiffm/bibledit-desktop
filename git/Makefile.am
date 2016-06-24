bin_PROGRAMS = bibledit-git

bibledit_git_SOURCES = bibledit-git.cpp

AM_CXXFLAGS = -Wall
AM_CXXFLAGS += $(GLIB_CFLAGS)
AM_CXXFLAGS += $(GTK3_CFLAGS)

LIBS = -Wall 
LIBS += $(GLIB_LIBS)
LIBS += $(GTK3_LIBS)

if LINUX
endif
if DARWIN
endif
if WIN32
LIBS += -mwindows -lws2_32
endif

CLEANFILES = *~

EXTRA_DIST = *.h
