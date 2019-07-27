# this makefile is for the PD make on fish 69, using Aztec C 5.2b

# READ:  Just plain "make" makes a version of Dr called DD which is set up for
# debugging with either SDB or DB.  Use "make d" to make a finished version.

LIB = -lc16

PRECOMP = -hi II16

# Substitute whatever pathname you want to use for the file II16.  If you
# don't want to make precompiled header files just use "PRECOMP =".  II16
# can be precompiled from exec/exec.h, libraries/dosextens.h, the prototypes 
# and pragmas for the exec and dos libraries, and perhaps string.h and Paul.h,
# with 16 bit ints.  IPRECOMP is for all of those plus intuition/intuition.h and
# the intuition pragmas and prototypes.

SDBCMD = -bs -s0f0n

CCOPTS = -pe -wcrup -sabfmnpu
#  (I normally set the CCOPTS environment var to "-pe -wcl -sabfmnpu -qf".)

CFLAGS = $(CCOPTS) $(PRECOMP) -ps
C = $(CFLAGS)
L = +q -m


########## this produces a version of Dr that SDB or DB can be used on, which
########## will NOT be pure:

ram\:dd : ddr.o dsmallio.o
	ln $L -w -g -o ram:dd ddr.o dsmallio.o $(LIB)
	-@dr -l ram:dd\#?

# the last line above assumes you already have a working Dr on line.  And it
# also responds to the fact that this pd make considers # in the MIDDLE of a
# line to be a start of comment unless preceded by a backslash.

ddr.o : dr.c
	cc $C $(SDBCMD) -d DEBUG -o ddr.o dr.c

dsmallio.o : smallio.c
	cc $C $(SDBCMD) -d BACKGROUND -o dsmallio.o smallio.c


############## This is the final version without debugging information:

d : ram:Dr

ram\:Dr : dr.o purify.o smallio.o
	ln $L -o ram:Dr dr.o purify.o smallio.o $(LIB)
	@protect ram:Dr +p
	-@c:dr ram:Dr

dr.o : dr.c
	cc $C dr.c

smallio.o : smallio.c
	cc $C -d BACKGROUND smallio

n : NathanHale

NathanHale : t:nathanhale.o
	ln $L -o NathanHale t:nathanhale.o $(LIB)
	-@dr NathanHale

t\:nathanhale.o : nathanhale.c
	cc $C -o t:nathanhale.o nathanhale.c
