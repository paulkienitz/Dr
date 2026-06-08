/* this is stuff for calling fastscan.library from an Aztec or (hopefully)
SAS/Lattice or DICE C program.  if using an older non-ansi compiler, #define
__NO_ANSI and it will make do.  It will do that for you with Aztec and (I think)
with Lattice, when you have an older version. */


#ifndef FASTSCAN_DOT_AITCH
#define FASTSCAN_DOT_AITCH

#include <exec/types.h>
#include <libraries/dos.h>


struct Library *FastScanBase;


struct Fib {
    struct FileInfoBlock f;
    long p;
    long s;
};


#ifdef AZTEC_C
#if __VERSION < 500
#define __NO_ANSI
#endif
#endif

#ifdef LATTICE
#ifndef LATTICE_50
#define __NO_ANSI
#endif
#endif


#ifdef __NO_ANSI

long FastExamine();
long FastExNext();
void FastExCleanup();
void *FastExGet80();
long FSRexxQuery();

#else

long FastExamine(BPTR l, struct Fib *fib);
long FastExNext(BPTR l, struct Fib *fib);

void FastExCleanup(struct Fib *fib);
void *FastExGet80(struct Fib *fib);

long FSRexxQuery(struct Message *rxm);     /* currently just returns error 1 */


#ifdef AZTEC_C

#pragma amicall(FastScanBase, 0x1E, FSRexxQuery(a0))
#pragma amicall(FastScanBase, 0x24, FastExamine(d0, a0))
#pragma amicall(FastScanBase, 0x2A, FastExNext(d0, a0))
#pragma amicall(FastScanBase, 0x30, FastExCleanup(a0))
#pragma amicall(FastScanBase, 0x36, FastExGet80(a0))

#endif


#ifdef LATTICE

#pragma libcall FastScanBase FSRexxQuery   1E 801
#pragma libcall FastScanBase FastExamine   24 8002
#pragma libcall FastScanBase FastExNext    2A 8002
#pragma libcall FastScanBase FastExCleanup 30 801
#pragma libcall FastScanBase FastExGet80   36 801

#endif


/*** DICE has no applicable pragmas at this time --
     use "#ifdef _DCC" when it does, I think ***/

#endif

#endif

