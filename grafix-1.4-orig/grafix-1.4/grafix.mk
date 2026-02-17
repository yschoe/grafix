#   grafix.mk  wolf 11/94
#              to be included in the Makefiles
#
# used for OS-adaptation : set the following macros 
#	GCCINC	: path for gcc-lib includes (eg "regex.h")
#	XPATH  	: path for X11R6 (for includes and libs)
#
# uname = Linux : GCCINC not needed
# uname = SunOS : GCCINC : in ./ copied


TAR = tar
GCCINC = ./gccinc

ifeq ($(shell uname),Linux)
  XPATH = /usr/X11R6
  LFLAGS = -L$(XPATH)/lib
  CFLAGS = -Wall -I$(GCCINC) -DLINUX 
endif

ifeq ($(shell uname),SunOS)
  XPATH = /opt/X11R6

  ifeq ($(shell hostname),champ10)
    XPATH = /usr/openwin
  endif

  CFLAGS = -I$(XPATH)/include -I$(GCCINC) -O2 -DSPARC
  LFLAGS = -L$(XPATH)/lib
  TAR = gtar 
endif

ifeq ($(shell uname),HP-UX)
  XPATH = /usr
  LFLAGS = -L$(XPATH)/lib/X11R5 
  CFLAGS = -I$(XPATH)/include/X11R5 -I$(GCCINC) -DHP
endif

# CFLAGS += -DNDEBUG for final release
CFLAGS += -c -O2
LFLAGS += -lm -lX11

CC = g++
AR = ar rc

# clear suffixes list
.SUFFIXES:

# pattern rules: 
%.o:	%.C
	$(CC) $(CFLAGS) $<

%.o:	%.c
	$(CC) $(CFLAGS) $<

%: %.o
	$(CC) -o $@ $^ $(LFLAGS) 

