# This makefile is for the PD make on fish 69, using Aztec C 5.2b.  This
# first variable specifies the version number that Dr will get:

V = 2.1

# Just plain "make" makes a version of Dr called DD which is set up for
# debugging with either SDB or DB.  Use "make d" to make a finished version.
# Neither will work unless you have VersionDate available -- use "make v" to
# create it.  The extra program NathanHale can be created with "make n".

LIB = -lc16

PRECOMP = -hi II16

# Substitute whatever pathname you want to use for the file II16.  If you
# don't want to make precompiled header files just use "PRECOMP =".  II16
# can be precompiled from exec/exec.h, libraries/dosextens.h, the prototypes 
# and pragmas for the exec and dos libraries, and perhaps string.h and Paul.h,
# with 16 bit ints.

SDBCMD = -bs -s0f0n

CCOPTS = -pe -wcpru -sabfmnpu
#  (I normally set the CCOPTS environment var to those same options plus -qf.)

CFLAGS = $(CCOPTS) $(PRECOMP) -ps
C = $(CFLAGS)
L = +q -m


########## this produces a version of Dr that SDB or DB can be used on, which
########## will NOT be pure:

ram\:dd : ddr.o dsmallio.o
	ln $L -w -g -o ram:dd ddr.o dsmallio.o $(LIB)
	-@dr -l ram:dd\#?

# the last line above assumes you already have a working Dr on line.  And it
# also responds to the fact that this pd make considers # in the middle of a
# line to be a start of comment unless preceded by a backslash.

ddr.o : dr.c
	VersionDate > env:DrVersion Dr $V
	cc $C $(SDBCMD) -d DEBUG -o ddr.o dr.c

dsmallio.o : smallio.c
	cc $C $(SDBCMD) -d BACKGROUND -o dsmallio.o smallio.c


############## This is the final version without debugging information:

d : ram:Dr

ram\:Dr : dr.o smallio.o purify.o
	ln $L -o ram:Dr dr.o smallio.o purify.o $(LIB)
	@protect ram:Dr +p
	-@c:dr ram:Dr

dr.o : dr.c
	VersionDate > env:DrVersion Dr $V
	cc $C dr.c

smallio.o : smallio.c
	cc $C -d BACKGROUND smallio

v : VersionDate

n : NathanHale

VersionDate : t:versiondate.o
	ln $L -o VersionDate t:version purify.o $(LIB)
	-@dr VersionDate

t\:versiondate.o : versiondate.c
	cc $C -o t:versiondate.o

NathanHale : t:nathanhale.o
	ln $L -o NathanHale t:nathanhale.o $(LIB)
	-@dr NathanHale

t\:nathanhale.o : nathanhale.c
	cc $C -o t:nathanhale.o nathanhale.c

purify.o : purify.a
	as purify.a
