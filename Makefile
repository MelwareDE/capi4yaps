#	-*- sh -*-
#
include	Config
#
#CFLAGS	:= -include mem.h $(CFLAGS)
CFLAGS	:= $(CFLAGS)
#
DEFS	= -DCFGFILE=\"$(YAPS_CFGFILE)\" -DLCFGFILE=\"$(YAPS_LCFGFILE)\" \
	  -DLIBDIR=\"$(YAPS_LIBDIR)\"
LIBS	= -L. -lpager $(LLIBS) -lcapi20
OBJS	= data.o util.o cfg.o tty.o cv.o asc.o scr.o tap.o ucp.o \
	  slang.o lua.o capiconn.o #mem.o
YOBJS	= yaps.o valid.o
DSTFLE	= $(YAPS_BINDIR)/yaps

all:	yaps yaps.doc
	@if [ -f contrib/Makefile ]; then	\
		$(MAKE) -C contrib all ;	\
	fi

yaps:	libpager.a $(YOBJS)
	$(CC) $(LDFLAGS) $(YOBJS) -o $@ $(LIBS)

libpager.a:	$(OBJS)
	rm -f $@
	ar rc $@ $(OBJS)
	ranlib $@

yaps.o:	yaps.c pager.h
	$(CC) $(CFLAGS) $(DEFS) $< -c -o $@

yaps.doc:	yaps.html
	lynx -cfg=/dev/null -nolist -dump $< > $@

install:	$(DSTFLE) $(CFGFILE)
	if [ ! -d $(YAPS_LIBDIR) ]; then \
		install -d -m 755 -o $(YAPS_USER) -g $(YAPS_GROUP) $(YAPS_LIBDIR) ; \
	fi
	@if [ -f contrib/Makefile ]; then	\
		$(MAKE) -C contrib install ;	\
	fi

$(DSTFLE):	yaps
	install -o $(YAPS_USER) -g $(YAPS_GROUP) -m $(YAPS_MODE) -s yaps $@

$(CFGFILE):	yaps.rc
	@if [ ! -f $@ ]; then \
		install -o $(YAPS_RCUSER) -g $(YAPS_RCGROUP) -m $(YAPS_RCMODE) -s yaps.rc $@ ; \
	fi

clean:
	rm -f *.[oa] *~ .*~ yaps core .depend
	@if [ -f contrib/Makefile ]; then	\
		$(MAKE) -C contrib clean ;	\
	fi

depend:
	$(CPP) $(CFLAGS) -MM *.c > .depend
	@if [ -f contrib/Makefile ]; then	\
		$(MAKE) -C contrib depend ;	\
	fi

ifeq (.depend,$(wildcard .depend))
include .depend
endif
