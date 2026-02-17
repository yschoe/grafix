# Makefile for grafix-applications  
#	wolf 6/94

include grafix.mk

DEMOS = class-browser earth hello win-demo edit-demo file-browser cursors calc \
	dir-tree font-browser \
	pal-demo lat-demo lat2demo one-dim two-dim clock-demo three-dim \
	replay mandel

PROGS = scrolled_demo 

ifeq ($(shell uname),Linux) 
	PROGS += hardcopy 
endif

TOSHAR = window.c window.h lat_win.c lat_man.c reg_man.c lattice.h \
	palette.c palette.h files.c files.h smolark.c smolark.h solver.c \
	font-selector.c font-selector.h \
	mapdata.c hardcopy.c \
	animator.h animator.c wave.c three-nxnynz.h \
	icon.h eventnames.h README.txt HOWTO.txt COPYING LOG.txt TODO.txt \
	FAQ.txt Makefile grafix.mk \
	calc.icon tree.h tree.c tree_icon.h gccinc \
	$(DEMOS:%=%.c) $(PROGS:%=%.c) 

all:	demos $(PROGS)
allnew: depend all

demos: 	$(DEMOS)

test: 	clean depend all tgz test-tgz

# make demos = ~ 100 sec P90

# the classes to lattice.h are located in files lat_win, lat_man, reg_man
LATTICE = lat_win.o lat_man.o reg_man.o

libwin.a: window.o palette.o $(LATTICE) 
	$(AR) libwin.a $?
	ranlib libwin.a

earth:		earth.o mapdata.o window.o
mandel:		mandel.o window.o
font-browser: 	font-browser.o window.o font-selector.o files.o
hardcopy:	hardcopy.o window.o
hello: 		hello.o window.o
win-demo: 	win-demo.o window.o
edit-demo: 	edit-demo.o window.o
cursors: 	cursors.o window.o
calc: 		calc.o window.o
pal-demo: 	pal-demo.o libwin.a
lat-demo: 	lat-demo.o libwin.a
lat2demo: 	lat2demo.o libwin.a
one-dim: 	one-dim.o solver.o libwin.a
two-dim: 	two-dim.o smolark.o libwin.a
three-dim: 	three-dim.o wave.o animator.o files.o libwin.a
file-browser: 	file-browser.o files.o window.o
clock-demo: 	clock-demo.o window.o
replay: 	replay.o animator.o  files.o libwin.a
tree: 		tree.o window.o files.o
scrolled_demo: 	scrolled_demo.o window.o 
class-browser: 	class-browser.o tree.o window.o files.o
dir-tree: 	dir-tree.o tree.o window.o files.o

clean:
	\rm -f *.o *.a $(DEMOS) $(PROGS)
	\rm -rf ttgz/*
	\rm -rf tshar/*

masterproper: clean
	\rm -f .depend

####### make shell archives for comp.sources.misc -> subdir post #####
shar:
	shar -o Grafix.shar -l 100 -n Grafix  -a -s wolf@first.gmd.de \
	$(TOSHAR)
	cat intro.txt Grafix.shar.01 > tmp; mv -f tmp Grafix.shar.01

test-shar:
	\rm -rf tshar/*
	cp *.shar* tshar/
	cd tshar; unshar *shar*; $(MAKE) demos;

###### generate tar-file for uploading to sunsite #####
# "make tgz" - generates grafix.tar.gz file
# NEWDIR is a symbolic link into the actual directory, used as destination
# directory for expanding on other systems

VERSION = 1.4
NEWDIR  = grafix-$(VERSION)
TARFILE = $(NEWDIR).tar.gz
tgz: 
	ln -s . $(NEWDIR)
	$(TAR) -czh -f $(NEWDIR)/$(TARFILE) $(TOSHAR:%=$(NEWDIR)/%) 
	uuencode $(TARFILE) $(TARFILE) > $(TARFILE).uu
	\rm $(NEWDIR)

test-tgz:
	$(TAR) -xvzf $(TARFILE); 
	cd $(NEWDIR); $(MAKE) allnew;


# runs all the demos :
demorun: $(DEMOS)
	@for demo in $(DEMOS); do (echo "running $$demo"; exec $$demo;); done

# make dependencies:
depend:	
	$(CC) -MM $(CFLAGS) *.c > .depend

-include	.depend


