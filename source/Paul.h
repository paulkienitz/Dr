/*
The following declarations are used in most every program written by Paul
Kienitz.  Generally, I use a symbol table file which includes exec/exec.h,
libraries/dosextens.h, and string.h, and exec and dos prototypes.  I have
two versions: one includes only that and the other adds intuition/intuition.h
and most of the other prototypes.

This include file is no longer usable with pre-ANSI Aztec.  If Aztec is used it
must be 5.0 or newer.

Note to myself: modern SAS/C defines LATTICE, LATTICE_50, SASC, SASC_510, and
__VERSION__ = 5 or 6.  Aztec 5.2 sets __VERSION__ to 500.
*/

#ifndef PAUL_DOT_AITCH
#define PAUL_DOT_AITCH

#ifdef __STDIO_H
#  define put(S) fputs(S, stdout)
#endif


#if __STDC__
#  define _ANSI_C
#else
#  ifdef AZTEC_C
#    define _ANSI_C
#  endif
#endif


#ifdef _ANSI_C
#  define _PROTO(X) X
#  define VOLATILE volatile
#  define CONST const
#  define SIGNED signed
#else
#  define _PROTO(X) ()
#  define VOLATILE 
#  define CONST
#  define SIGNED
#endif


#ifdef AZTEC_C

#  ifdef INTUITION_INTUITION_H

#    define OpenWorkBench       ____00argleBArGlE__

#    include <functions.h>	/* define inline calls of most rom functions */

#    undef OpenWorkBench
#    undef GetDefaultPubScreen
struct Screen *OpenWorkBench(void);			/* BOOL in clib/... */
#    pragma amicall(IntuitionBase, 0xD2, OpenWorkBench())

#  else

#    include <clib/exec_protos.h>
#    include <clib/dos_protos.h>
#    include <clib/timer_protos.h>
#    include <clib/diskfont_protos.h>
#    include <pragmas/exec_lib.h>
#    include <pragmas/dos_lib.h>
#    include <pragmas/timer_lib.h>
#    include <pragmas/diskfont_lib.h>

/* with 2.0 includes those are about all of the commonly needed ones which we
can manage to pull in without bringing the weight of intuition/intuition.h
crashing down on our heads. */

#  endif

#endif


#ifdef LIBRARIES_MATHFFP_H

/* too many Commodore includes which have no need for floating point pull this
in.  It #defines a bunch of floating point functions so that it becomes
impossible to use the linker library functions of the same name. */

#  undef sin
#  undef cos
#  undef tan
#  undef asin
#  undef acos
#  undef atan
#  undef sinh
#  undef cosh
#  undef tanh
#  undef exp
#  undef log
#  undef log10
#  undef pow
#  undef sqrt
#  undef fabs
#  undef floor
#  undef ceil

#endif


/* This stuff has to be BELOW functions.h now or the #defines get it confused: */

#define null   ((void *) 0L)
#define maxint 0x7FFFFFFFL
#define minint 0x80000000L
/* uppercase is a pain in the wazoo */

#define bip(T, B) ((T *) ((B) << 2))
#define gbip(B)   bip(void, B)
/* B is expected to be of type BPTR or BSTR. */

typedef short bool;		/* change this to ubyte and see what breaks? */
#define false ((bool) 0)
#define true  ((bool) 1)

typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned char  ubyte;

typedef void *adr;
typedef SIGNED char *str;
typedef ubyte *ustr;

typedef void (*PVF)();		/* pointer to void function */
typedef int  (*PIF)();		/* pointer to function returning int */


#define bit(B) (1L << (B))

#define import extern
#define PUBLIC
#ifndef private
#  define private static
#endif


#ifndef _SIZE_T
#  define _SIZE_T
typedef unsigned long size_t;
#endif


/* AND NOW, convenience stuff:::::::::::::::::::: */

#ifdef EXEC_MEMORY_H

#  define MEMF_CP     (MEMF_CHIP | MEMF_PUBLIC)
#  define MEMF_CZ     (MEMF_CHIP | MEMF_CLEAR)
#  define MEMF_PZ     (MEMF_PUBLIC | MEMF_CLEAR)
#  define MEMF_CPZ    (MEMF_CHIP | MEMF_PUBLIC | MEMF_CLEAR)

#  define Alloc(S)    AllocMem((ulong) (S), 0L)
#  define AllocC(S)   AllocMem((ulong) (S), MEMF_CHIP)
#  define AllocP(S)   AllocMem((ulong) (S), MEMF_PUBLIC)
#  define AllocZ(S)   AllocMem((ulong) (S), MEMF_CLEAR)
#  define AllocCP(S)  AllocMem((ulong) (S), MEMF_CP)
#  define AllocCZ(S)  AllocMem((ulong) (S), MEMF_CZ)
#  define AllocPZ(S)  AllocMem((ulong) (S), MEMF_PZ)
#  define AllocCPZ(S) AllocMem((ulong) (S), MEMF_CPZ)
#  define NEW(A)      (A = AllocMem(sizeof(*(A)), 0L))
#  define NEWC(A)     (A = AllocMem(sizeof(*(A)), MEMF_CHIP))
#  define NEWP(A)     (A = AllocMem(sizeof(*(A)), MEMF_PUBLIC))
#  define NEWZ(A)     (A = AllocMem(sizeof(*(A)), MEMF_CLEAR))
#  define NEWCP(A)    (A = AllocMem(sizeof(*(A)), MEMF_CP))
#  define NEWCZ(A)    (A = AllocMem(sizeof(*(A)), MEMF_CZ))
#  define NEWPZ(A)    (A = AllocMem(sizeof(*(A)), MEMF_PZ))
#  define NEWCPZ(A)   (A = AllocMem(sizeof(*(A)), MEMF_CPZ))

#  ifdef PAUL_OLD_NEW
#    define New(T)      ((T *) AllocMem(sizeof(T), 0L))
#    define MewC(T)     ((T *) AllocMem(sizeof(T), MEMF_CHIP))
#    define NewP(T)     ((T *) AllocMem(sizeof(T), MEMF_PUBLIC))
#    define NewZ(T)     ((T *) AllocMem(sizeof(T), MEMF_CLEAR))
#    define NewCP(T)    ((T *) AllocMem(sizeof(T), MEMF_CP))
#    define NewCZ(T)    ((T *) AllocMem(sizeof(T), MEMF_CZ))
#    define NewPZ(T)    ((T *) AllocMem(sizeof(T), MEMF_PZ))
#    define NewCPZ(T)   ((T *) AllocMem(sizeof(T), MEMF_CPZ))
#    define Free(T, A)  FreeMem(A, sizeof(T))
#  endif

#endif

#define FREE(A)     FreeMem(A, sizeof(*(A)))


#ifdef DOS_DOS_H
#  define LIBRARIES_DOS_H
#endif
#ifdef DOS_DOSEXTENS_H
#  define LIBRARIES_DOSEXTENS_H
#endif

#ifdef LIBRARIES_DOS_H

#  define RLock(F) Lock((ustr) F, (long) ACCESS_READ)
#  define WLock(F) Lock((ustr) F, (long) ACCESS_WRITE)
#  define OOpen(F) Open((ustr) F, (long) MODE_OLDFILE)
#  define NOpen(F) Open((ustr) F, (long) MODE_NEWFILE)

#endif

#define OpenL(n) (adr) OldOpenLibrary((ustr) n ".library")

#define Forbid() ((*((ubyte **) 4))[295]++)
/* as efficient as assembly; Aztec turns that into two instructions. */

#ifdef LIBRARIES_DOSEXTENS_H

#  define ThisProcess() ((*((struct Process ***) 4))[69])
/* equivalent to (struct Process *) SysBase->ThisTask, two instructions. */

#endif


#ifdef AZTEC_C
#  define strcpy _BUILTIN_strcpy
#  define strcmp _BUILTIN_strcmp
#  define strlen _BUILTIN_strlen
str strcpy(str _dst, const str _src);
int strcmp(const str _s1, const str _s2);
size_t strlen(const str _s);
#endif

#ifndef offsetof
#  define offsetof(type,memb) ((size_t)((unsigned long)&((type *)0)->memb))
#endif

#endif PAUL_DOT_AITCH
