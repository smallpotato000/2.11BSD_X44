include ../Make.inc
include ../unix/Make.$(SYSNAME)

PREFIX=/usr/local

NUKEFILES=

TGZFILES=

CLEANFILES=

LIB=libutf.a
VERSION=2.0
PORTPLACE=devel/libutf
NAME=libutf

OFILES=\
	rune.$O\
	runestrcat.$O\
	runestrchr.$O\
	runestrcmp.$O\
	runestrcpy.$O\
	runestrdup.$O\
	runestrlen.$O\
	runestrecpy.$O\
	runestrncat.$O\
	runestrncmp.$O\
	runestrncpy.$O\
	runestrrchr.$O\
	runestrstr.$O\
	runetype.$O\
	utfecpy.$O\
	utflen.$O\
	utfnlen.$O\
	utfrrune.$O\
	utfrune.$O\
	utfutf.$O\

HFILES=\
	utf.h\

all: $(LIB)

install: $(LIB)
	mkdir -p $(PREFIX)/share/man/man3 $(PREFIX)/man/man7
	install -c -m 0644 isalpharune.3 $(PREFIX)/share/man/man3/isalpharune.3
	install -c -m 0644 utf.7 $(PREFIX)/man/man7/utf.7
	install -c -m 0644 rune.3 $(PREFIX)/share/man/man3/rune.3
	install -c -m 0644 runestrcat.3 $(PREFIX)/share/man/man3/runestrcat.3
	mkdir -p $(PREFIX)/include
	install -c -m 0644 utf.h $(PREFIX)/include/utf.h
	mkdir -p $(PREFIX)/lib
	install -c -m 0644 $(LIB) $(PREFIX)/lib/$(LIB)

include ../Make.clib
include ../unix/Make.package
