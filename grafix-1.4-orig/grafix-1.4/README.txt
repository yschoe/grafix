Grafix 	Ver. 1.4		7/97
=====================================

0. Installation
===============
a) gtar -xvzf grafix-1.4.tar.gz 
   (if you don't have "gtar" or GNUs "tar" version, use "gunzip" first and then 
    invoke "tar -xvf grafix")
   This expands the tar file into some subdir like "grafix-1.4"
   cd to this subdir and:

b) check "grafix.mk" for the correct paths for Xlib (X11R6 or X11R5) -> XPATH
   and some include directories on your system. 
   Please, read the following text if you have any problems.

c) gmake allnew : to build all demo programs
   (if you get compiling errors, see below)
 
d) gmake demorun : to invoke a run of all demos

1. What is "Grafix" ?
=====================
"Grafix" is a utility originally designed to help scientists in the 
visualization of results of a computation, e.g. for numerical integrations of 
partial differential equations. 

It can be used, however, for any application that wants to use X-Windows for 
drawing pictures, functions or other graphic objects in a convinient 
interactive manner or even for writing a graphical user interface for any task.

Grafix should be considered as a layer between an application and the X Window
system built up of a bunch of basic classes as building elements. 
The simplest way of using it is to define instances of these classes. 
For more complicated programs the user has to define own derivations to the
basic clases.

- Grafix is based on Unix and the X Window system.

- Grafix does not use any commercial code, like Motif, so it is totally free.

- Grafix is been ported and tested for Linux and SunOS, with X11R5/R6
  
- Grafix is small and fast since it is pure functionality without any 
  superfluous additives. 

- Grafix is completely written in C++ and to compile with g++ on both platforms
  The object-oriented approach enables a quite simple way of introducing user
  defined behaviour.

- Grafix includes classes for the basic operations to have a convinient window
  management, like : 
   - windows with automatic restoring for complex drawings (backing store)
   - several types of predefined button classes for different purposes :
     quit_button, delete_button, help_button, callback_button, toggle_button,
     instance_button, dump_button, hardcopy_button, ...
     all the buttons have a Motif-like three dimensional shape
   - popup-windows and pulldown-menus for selecting discrete values
   - help popups can be bound to any window
   - scrollbars for selecting coniguous values
   - windows with real-valued co-ordinate systems where the user has not to 
     worry about pixel co-ordinates
   - a simple edit window for entering strings
   - simple file selection boxes for openening files interactively
     for reading (with directory scan) and writing (query before overwrite)
   - a predefined palette manager for color definitions that can be 
     implemented in any application
   - as advanced class : an complete manager to handle the display
     of 2-dim functions (given on a equidistant grid) as lattice or body
     in arbitrary perpective, shadowing and details zooming.
   - new : animator class : to store time sequences of two dimensional arrays
     in a file (more than one, also 3-dim arrays can be treated as vector
     of 2-dim arrays) and replay them like a video film.
     This is at most useful for visualizing time consuming integration 
     procedures
   - new : integrator class : very easy to use prototype to link any
     numerical 2- or 3-dim FDE-integration program with the graphic interface
     and playback feature
   - new : scrolled windows of (nearly) arbitrary virtual size
   - new : classes for displaying tree- and graph-structures, and as 
     application a very nice graphic class browser 
     (and a simple one "dir-tree")

- Grafix has some example programs to show the basic functions,eg 
  "win-demo", "edit-demo", "file-browser", "calc", "pal-demo", etc.
  The simple ones should help the learner to understand the functionality :
     hello, win-demo, edit-demo, file-browser, pal-demo, clock-demo, dir-tree
  while the other provide also real functionality :
     class-browser, file-browser, cursors, calc, one-dim, two-dim, replay 
     
- The programs "one-dim", "two-dim", "three-dim", "replay"
  give a complete model of some numerical integration methods for the one
  or two dimensional advection equation (simple upstream method, 
  Smolarkievicz-method, Lax-Wendroff). They have several buttons and menus
  for playing with some parameters and three-dim allows 
  to afterwards show the ongoing integration as animation.
  These demos should be easily adopted to any one or two dimensional 
  set of differentional equation with time dependence (finite difference)

2. What is it NOT
=================
- it is not a commercial software, so I will not take any warranty for the 
  usability of the software
- it is not in a "ready to use" state, it requires the user to write own 
  C++-code (this should be obvious)
- I can give no guarantee to respond on error reports or queries for
  improvements since my time for this task is limited. But I'll try my best
  to help you.

3. What do you need to use "Grafix"
===================================
- You should have basic knowledge about C++ classes and what an X-window is.
  Further knowledge about Xlib programming will be helpful but is not 
  necessary since I tried to encapsulate this in the classes.
 
- You need g++ to compile the code, other compilers possibly do not allow some 
  special constructs for passing multidimensional arrays as parameters. 
  These type casts could be easily substituted, however.

- You need Unix (SunOS, Linux) and X11R5 or X11R6. 
  Portations to other platforms are not my concern, since I have no 
  possibility to test them. But if you have gcc and X11 there should be no
  major problems. 
  Reported platforms that work (possibly with minor adaptions): 
	SGI (Silicon Graphics, IRIX)
	Sun , SunOS 5.3, Solaris 2.3, Solaris 2.4
	Bull DPX/20 (PowerPC and AIX 3.2.5).
	PC (Interactive Unix, ISC 4.0)
     	HP 9000/735 with HP/UX 9.03A 	

- you have to agree to the  "GNU GENERAL PUBLIC LICENSE"
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION,
  (see included standard file "COPYING" for details)
  in order to use the program, copy, distribute or modificate it or parts
  of the code. (If you intend to use it for proprietary programs,
  please contact me, so we may find a solution.)  
  Additionally, I forbid the use of any part of the package
  for military purposes and any portations to Win*ows-XX

4. How to start
===============
I strongly suggest starting with the example programs in the package.
They demonstrate nearly the complete functionality. Compile them, start
and play with them. Then have a look at the sources and try to understand
how they work. 

- Have a look at the file "grafix.mk" if the paths for Xlib are correct for
  your platform. On my Linux-PC I have "XPATH = /usr/X11R6", while the Sun
  expects "XPATH = /opt/X11R6". BTW, X11R5 should also work.

  If you want to make a port to a new hardware platform, please add
  a new entry similiar to to the existing ones, eg.
	ifeq ($(shell uname),myplatform)
	XPATH = ....
	LFLAGS = ....
	CFLAGS = -I.... -DMYPLATFORM 
	endif
  If possible, let me know about this, so I may include this in the next 
  release.

  Another possible cause of compilation failure is that some special include
  files for gcc are not found (regex.h). Then you should include 
  "-I./gccinc" in your CFLAGS

- To build all demos type :
  for the first time "gmake depend" 
  and then "gmake"

- To let them all run consecutivly type :
  "gmake demorun"

- read the file HOWTO for a detailed instruction of the installation
  and a description of the use of Grafix.

5. Help sought !
================
Some extensive manuals would be very helpful for users, but I have no time 
to write them. It would be nice to have eg. class descriptions and more
general info, possibly as an info-file or other online help (html-manual ?). 
If anyone has the possibility to help me, please contact me per email.

6. Main changes since version 1.0 (from Feb. 95)
================================================
(see LOG.txt for details)

- event handling simplified, so  each window class receives only the events
	of interest
- new classes for text viewing (files.c)
- file-browser demo included
- clock_win : included
- main_window class allows additional arguments for positioning
- animator- and replay- functionality for saving and redisplaying results of
	integration runs
- scrolled windows included
- Tree windows derivation 
- class-browser as a complete demo for these
- dir-tree as simple demo for scrolled windows, and Tree windows
- some bug fixes : eg. the black background color on some displays
- compilation errorfree with gcc 2.6.3, and 2.7.0
- support of TrueColor modes (16bpp, 32bpp)

7. Main changes since version 1.2 (4.11.95), see LOG.txt
========================================================
(version 1.3 was an internal version and not released)

- in window.c : a hash table algorithm for mapping X-IDs to window pointers 
  now there is no more limit of the number of windows and they may be 
  constructed and deleted in any fashion !!!
- lattice : an isoline display mode implemented
- great new demos : 
  "earth" - rotating earth, 
  "font-browser" - browse through all X-fonts
  "mandel" - the famous Mandelbrot-Set
- some minor bugfixes, improvements and additions
- installation : the tar-File (tar.gz) now expands automatically into a new 
  directory "grafix-1.4"

8. Author
=========

----------------------------------------------------------------------------
    ... always look on the bright side of life ... (Monty Python)
----------------------------------------------------------------------------
Wolfgang Koehler                                       wolfk@gfz-potsdam.de
GeoForschungsZentrum Potsdam, Germany, Projekt CHAMP	Tel: (0331)2817928




