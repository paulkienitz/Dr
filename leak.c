/* This here is a tool for chasing down memory leaks.  As written it requires
that the program be using my PureIO module (pureio.c).  This can be changed.
By Paul Kienitz 7/90 public domain.  Output is compatible, more or less, with my
separate PSnoop program.
*/

/* What ya do is put macros in your shit sorta like this

#define AllocMem(a, b) AllocYell((long) a, (long) b, __FUNC__, (long) __LINE__)
#define FreeMem(a, b) FreeYell(a, (long) b, __FUNC__, (long) __LINE__)
void *AllocYell(long, long), FreeYell(void *);

   or if you're using Paul.h you should put these in first:

#undef FreeMem
#define _AllocMem(a, b) AllocYell((long) a, (long) b, __FUNC__, (long) __LINE__)
#define _FreeMem(a, b) FreeYell(a, (long) b, __FUNC__, (long) __LINE__)

   and it'll tell you about all the memory you allocate and free.  It'll let
   you know in what order which calls allocate what.  Then you just need some
   kinda filter to spot mismatches and you're all set.  I'll make one out of
   Uedit.  Something like this:

Match up allocation/deallocation reports in program output
<ramiga-5:	movecursor(curfile, sfile)
		if (not insertchar(curfile, eline)) {
		    putmsg("Cannot be used on read only buffer!")
		    returnfalse
		}
		putmsg("Checking...  (PSnoop style)")
		movecursor(curfile, sfile)
		getsearch(buf49)
		equatenum(n0, 0)
		setsearch("## Alloc'd")
		while (search(curfile, sinvert, einvert, 1)) {
		    equateloc(curfile, sinvert, einvert)
		    while (not is(curfile, ";")) movecursor(curfile, echar)
		    equateloc(curfile, einvert, atcursor)
		    clearbuf(buf54)
		    insertrgn(buf54, efile, curfile, invert)
		    insertrgn(buf54, sfile, "%% Freed  ", all)
		    setsearch(buf54)
		    equateloc(curfile, mouseloc, sinvert)
		    movecursor(curfile, einvert)
		    if (search(curfile, sinvert, einvert, 1)) {
			swapchar(curfile, "-")
			movecursor(curfile, mouseloc)
			movecursor(curfile, sline)
			swapchar(curfile, "-")
		    } else {
			incnum(n0)
			putmsg("MISMATCH!  Still checking...")
			movecursor(curfile, mouseloc)
		    }
		setsearch("## Alloc'd")
		}
		movecursor(curfile, sfile)
		clearchar(curfile)
		if (eqnum(n0, 0)) {
		    setsearch("%% Freed  ")
		    if (search(curfile, sinvert, einvert, 1))
			putmsg("Unmatched deallocation!")
		    else
			putmsg("All allocations freed.")
		} else
		    putmsg("There's a memory leak!")
		setsearch(buf49)
>
 */

#include <exec/memory.h>
#include <Paul.h>


#undef put

import /* from pureio.c */ void put(), putfmt();
/* caller must OpenPureIO */

import adr _AllocMem(long, long);
import void _FreeMem(adr);


#define MC MEMF_CHIP
#define MF MEMF_FAST
#define MP MEMF_PUBLIC
#define MZ MEMF_CLEAR
#define ML MEMF_LARGEST



void pause()
{
    long trash;
    if (IsInteractive(Input()) && Output()) {
	Write(Output(), "Press RETURN: ", 14L);
	Read(Input(), &trash, 1L);
    }
}



adr AllocYell(s, f, y, l) long s, f; str y; long l;
{
    register adr foo = _AllocMem(s, f);
    union {
	long w;
	char n[4];
    } t;
    short i = 0;
    t.w = 0;
    if (f & MC)
	t.n[i++] = 'C';
    if (f & MP)
	t.n[i++] = 'P';
    if (f & MZ)
	t.n[i++] = 'Z';
    if (!i)
	t.w = 'any\0';
    if (foo)
	putfmt("## Alloc'd%8ld at 0x%lx; ", s, foo);
    else
	putfmt("++ FAILED to allocate %ld bytes ", s);
    putfmt("(%s), in %s line %ld, largest %ld.\n",
		t.n, y, l, AvailMem(ML));
    if (!foo) {
	putfmt("++ AvailMem reports: CHIP %ld total %ld largest,\n     "
		"FAST %ld total %ld largest.\n", AvailMem(MC),
		AvailMem(MC | ML), AvailMem(MF), AvailMem(MF | ML));
	pause();
    }
    return foo;
}



void FreeYell(a, s, y, l) adr a; long s; str y; long l;
{
    if (!TypeOfMem(a) || s <= 0 || s >= 5000000) {
	putfmt("++!!  BOGUS FREEMEM:\n   %ld bytes at "
		"0x%lx at line %ld in call %s.\n", s, a, l, y);
	pause();
	return;			/* DON'T free it! */
    }
    putfmt("%%%% Freed  %l8d at 0x%lx; in call %s line %ld.\n",
		s, a, y, l);
    _FreeMem(a, s);
}
