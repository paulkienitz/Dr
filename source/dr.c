/* TO DO:
option letters available: E G J Q W  .... maybe:
   q = supercompaQt columns
   g = aGe
   j = Jump into soft links when -R / ::?

Filter cases where a multi-assign was already covered by recursion?
Hard links in -r listing can end up with linkee's name??
Add \:::f and \:::p?  \" for quote if space in pathname?
Someday make -P default to -P~H instead of no mask, if C= ever does likewise.
Add date-string parsing to -A and -B with syntax -A(date time).  -Adays.hours?
Make it show both -X info and -R info when both are specified.
Make -P able to have or-ed sets of bits, like -p~as|~ap ?
-G shows file's age in days hh:mm:ss?
Option -Q for super-compact columnation?  Always use super-compact?
Avoid repeating "unknown option" for same letter?

Note: when -R goes into a soft link to a file (presently disabled), it shows
   the file as if it were inside the soft link, rather than being what it's
   linked to.
*/

/* ==========================================================================

The idea here is to make Yet Another Cli Directory Command.  What's special
about this one?  It's really really fast, it's got more goddamn features than
you can believe, and by default it doesn't show .info files.  Instead it just
shows all files that have .info's associated with them by writing the name in
orange instead of white when output is to the screen.  And it puts file names
(and directory names also) in as many columns (up to ten) as will fit
comfortably in the window.  And everything is alphabetized in columns.  It will
also do Amiga patterns.  It tries a pattern first as an exact literal before
expanding it, in case you have a drawer named "Doesn't work?" or something.  And
it does recursive descents, faster than the Fast File System.  It is intended to
replace Dir, List, rls, and du, and to outperform all competitors.

Dr is written for Aztec C 5.2b for Amiga, by Paul Kienitz.  Public Domain.  See
the files Dr.doc and FastExNext.doc for useful information.  Documentation for
smallio.c is in the source, kind of.  Look in pureio.c (included with older
versions of Dr) if you really want to know more.
========================================================================== */

/* some #defines which you can make different verisons with:
	if C_NOT_ASM it does not use inline assembly language instead of C
	    for some sorting functions (which won't work with Lattice/SAS).
	if DEBUG is defined it may produce extra error messages
	if MARKLINKS is defined then a -L style output will mark hard and soft
	    links with H> and S>.
*/

#define VERSION     "2.0"

#define FLACK       ':'        /* NOT USED */

#define WIDEFAULT   77

#define HYPH        0x96

#define HELP        0x9F

#define CSI         "\x9B"

#define MAXCOLS     10

#define PREPENGTH   300

#define LOOPDEPTH   30

#define STACKNEEDED (1500 + 400)

#define PATLIMIT    128

#define ABSIZE      300L

#define SOFTSIZE    288L

#define MULTILIMIT  50

#define TOPSTYLE    3
/* styles: 0 = dented, 1 = blankline, 2 = stupid dashes, 3 = mingled */

/* WIDEFAULT is the assumed output width when we can't measure the window.
   FLACK is char used to mark icon'd files in output.  Not used these days.
   HYPH is used to mark option arguments for mane.
   CSI is the one-char string that starts "escape sequences"; same as esc [.
   MAXCOLS is the max number of text columns to stack listed names in.
   PREPENGTH is the maximum length of pathnames labelling recursive levels.
   LOOPDEPTH is how far back we check for hard-link looping.
   STACKNEEDED is the amount of stack space needed to scan a directory.
   PATLIMIT is the maximum length of pattern strings.
   ABSIZE is the maximum length of paths converted by the -T option.
   SOFTSIZE is the number of characters allowed for a soft link's path string.
   MULTILIMIT is the number of locks that can be handled in a multiple assign.
*/


#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/datetime.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <pragmas/dos_lib.h>
#include <pragmas/exec_lib.h>
#include <ctype.h>
#include <string.h>
#include <Paul.h>

#ifdef put
#  undef put
#endif

/* Here we have a simplified version of <devices/conunit.h> which does not
pull in stuff like struct Window and struct TextFont: */

struct ConUnit {
    short pad[21];
    short cu_XMax, cu_YMax;
};
/* pretty simplified, wasn't it */


typedef struct _fly *flip;

typedef struct _fly {
    flip next;			/* list link */
    long length, blox, tection;
    struct DateStamp when;
    long keee;
    str comment;
    short ordination;
    char name[31];
    /* these are the most heavily used flags: */
    char /* bool */ jected, infoed, wanted, dirred, softlink;
    ushort fflags;	/* ... and these are the less used ones */
    str slinky;
} fly;			 /* size exactly 80 bytes always */

#define F_HARDLINK 1
#define F_DESCEND  2
#define F_ROOT     4


struct cuont {
    long blok, byt, fil, dir;
};


adr stacklimit;
str argline;
str *argv;
int arglen, argc, hyphc;

short cwid, song, keygits, cols, wid, abort, rdepth, steil = 0;

bool color, curse, cize, complete, cron, cons, cutdirs, ceys, cutfils, colorful;
bool /* cage, */ canydepth, csternal, cortless, consumption, ctifle, cweeek;
bool creverse, colsort, cizesort, cursorhide, convertpath, cancelenvar, cupside;
bool zize, zomplete, /* zons, FLACK */ zurse, zutdirs, zutfils, zortless;
bool zuttop, zutbot;

long protlook, protwant, before, after, tooday;

str oform, xform;

struct ConUnit *cuca;
struct Process *me;
long hair;

ubyte mesh[PATLIMIT * 2 + 3];
char abspath[ABSIZE];

struct AnchorPath *ankh;

bool patty, flipat, didaninny, needsnl, anyfiles, morethan1;
bool notadir, notadirbutokay;

#ifdef MARKLINKS
bool anylinks;
#endif

char prepath[PREPENGTH];


struct Ants {
    struct Ants *prav;
    long kay;
};


struct DosLibrary *DOSBase;


char versionstring[] = "\0$VER: Dr " VERSION " (" __DATE__ ")\n\r";

char helpslab[] =
"    -C sort from oldest to newest       -D show only dirs, not files\n"
"    -F show only files, not dirs        -H sort in rows, not columns\n"
"    -I show .info files normally        -K show disk addresses\n"
"    -L show size protection date etc.   -M don't use color to show icons\n"
"    -O one complete pathname per line   -R show contents of all subdirs\n"
"    -S show length of each file         -T convert paths to absolute form\n"
"    -U just show disk space usage       -V sort in reverse order\n"
"    -X show date etc, not contents      -Y use day names for recent dates\n"
"    -Z sort from smallest to largest    -? no output; return 5 if no match\n"
"    -@ don't check DR-OPTS variable     -! turn off cursor for faster output\n"
"    -A# (e.g. -A7) show files and dirs changed in the last # days\n"
"    -B# (e.g. -B30) show those changed more than # days ago\n"
"    -N# (e.g. -N3) select style for arranging output of files and dirs\n"
"    -Pb where b is one or more of H S P A R W E D with optional ~ in front\n"
"         of letters (e.g. -PS~A~D): don't show non-matching protection bits\n"
"    -[...string...] describes format of output -- see docs for description\n"
"         of special codes preceded with \"\\\".\n"
"    -{...string...} is like -[...] but result is executed as a command\n";


#define SIGMASK (long) (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D)


ubyte sorder[256] = "\0\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17"
		    "\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37"
		    " !\"#$%&'()*+,-./0123456789:;<=>?"
		    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
		    "`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~\177"
		    "\200\201\202\203\204\205\206\207"
		    "\210\211\212\213\214\215\216\217"
		    "\220\221\222\223\224\225\226\227"
		    "\230\231\232\233\234\235\236\237"
		    " ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿"
		    "AAAAAAACEEEEIIIIDNOOOOO×OUUUUYÞS"
		    "AAAAAAACEEEEIIIIDNOOOOO÷OUUUUYÞY";

/* for correct sorting: "Mädchen" between "madam" and "madman". */


struct pack {
    struct FileInfoBlock fibuf;
    struct StandardPacket sp;
    struct MsgPort repp;
    bool isout;
    BPTR lock;
};


/* ================== functions: ================== */


#pragma amicall(DOSBase, 0xf0, DoPkt3(d1,d2,d3,d4,d5))

#ifdef LEAKAGE

import adr AllocYell(long a, long b, str c, long d);
import void FreeYell(adr a, long b, str c, long d);
#define AllocMem(a, b) AllocYell((long) a, (long) b, __FUNC__, (long) __LINE__)
#define FreeMem(a, b) FreeYell(a, (long) b, __FUNC__, (long) __LINE__)
#define _AllocMem(a, b) AllocYell((long) a, (long) b, __FUNC__, (long) __LINE__)
#define _FreeMem(a, b) FreeYell(a, (long) b, __FUNC__, (long) __LINE__)

#endif


import bool OpenSmallIO(void (*ff)());
import void CloseSmallIO(void), puch(ushort c), put(str s),
		putfmt(str format, ...), pflush(void), StopAllOutput(void);


#define lower(C)  ((C) | 0x20)


/* for forward references: */

void DoInner(str n, BPTR l, bool hasparent,
			struct Ants *ants, struct cuont *great);
void Cough1(register flip y, bool dirs, short *col);
void FloorMat(register flip y, bool xeq);


private void btoc(str c, ubyte *b, short lim)		/* b == c is okay */
{
    register ushort l = *b;
    if (l > lim) l = lim;
    strncpy(c, (str) b + 1, (size_t) l);
    c[l] = '\0';
}



private void SendExNext(register struct pack *p)
{
    register struct DosPacket *d = &p->sp.sp_Pkt;
    register short l = strlen(p->fibuf.fib_FileName);
    register str s = &p->fibuf.fib_FileName[l];

    while (s > &p->fibuf.fib_FileName[0])	/* un-btoc() */
	*s = s[-1], s--;
    *(ubyte *) s = l;
    /* the filesystem ralphs if you send a null lock to pr_FileSystemTask? */
    p->sp.sp_Msg.mn_Node.ln_Name = (adr) d;
    d->dp_Link = &p->sp.sp_Msg;
    d->dp_Port = p->sp.sp_Msg.mn_ReplyPort = &p->repp;
    d->dp_Type = ACTION_EXAMINE_NEXT;
    d->dp_Arg1 = p->lock;
    d->dp_Arg2 = ((long) &p->fibuf) >> 2;
    p->isout = true;
    PutMsg(bip(struct FileLock, p->lock)->fl_Task, d->dp_Link);
}



private void WaitExNext(struct pack *p)
{
    register struct Message *m;
    while (p->isout) {
	WaitPort(&p->repp);
	while (m = GetMsg(&p->repp))
	    if (m == &p->sp.sp_Msg) {
		p->isout = false;
		break;
	    }
    }
    btoc(p->fibuf.fib_FileName, (ubyte *) p->fibuf.fib_FileName, 30);
    btoc(p->fibuf.fib_Comment, (ubyte *) p->fibuf.fib_Comment, 79);
}



void AsExCleanup(struct pack **pk)
{
    struct pack *p = *pk;

    if (p && ~(long) p) {
	WaitExNext(p);			/* you can't AbortIO a packet */
	FREE(p);
	*pk = (adr) ~0;
    }
}



long AsExamine(BPTR lok, struct FileInfoBlock *fibb, struct pack **pk)
{
    long r;

    *pk = null;
    r = Examine(lok, fibb);
    if (!lok || !r || fibb->fib_EntryType < 0)
	return r;
    if (NEWPZ(*pk)) {
	register struct pack *p = *pk;
	p->repp.mp_Node.ln_Type = NT_MSGPORT;
	p->repp.mp_Flags = PA_SIGNAL;
	p->repp.mp_SigBit = SIGB_DOS;
	p->repp.mp_SigTask = me;
	NewList(&p->repp.mp_MsgList);
	p->repp.mp_MsgList.lh_Type = NT_MSGPORT;	/* what the heck */
	p->fibuf = *fibb;		/* copy of struct, not pointer */
	p->isout = false;
	p->lock = lok;
	SendExNext(p);
    }
    return r;
}



long AsExNext(BPTR lok, struct FileInfoBlock *fibb, struct pack **pk)
{
    long ret;
    struct pack *p = *pk;

    if (!~(long) p) {
	me->pr_Result2 = ERROR_NO_MORE_ENTRIES;
	return 0;
    }
    if (!p)
	return ExNext(lok, fibb);
    WaitExNext(p);
    *fibb = p->fibuf;
    ret = p->sp.sp_Pkt.dp_Res1;
    me->pr_Result2 = p->sp.sp_Pkt.dp_Res2;
    if (!ret && me->pr_Result2 == ERROR_NO_MORE_ENTRIES)
	AsExCleanup(pk);
    else
	SendExNext(p);		/* continue in spite of error */
    return ret;
}



void putn(str s)
{
    put(s);
    puch('\n');
}



long StackLeft(void)
{
    short i;
    return (long) &i + 14 - (long) stacklimit;
}



short digits(register ulong l)
{
    register short d = 0;
    while (l) {
	d++;
	l /= 10;
    }
    return (d ? d : 1);
}



void pad(short w)
{
    while (--w >= 0)
	puch(' ');
}



void padong(ulong n, short w)
{
    pad(w - digits(n));		     /* RawDoFmt doesn't have %*ld */
    putfmt("%ld", n);
}
/* RawDoFmt doesn't have %lu either, so let's just hope n is always positive */



void fortection(str s, register ulong bits)
{
    register short b;

    bits ^= 15;
    strcpy(s, "hsparwed");
    for (b = 0; b <= 7; b++)
	if (!(bits & bit(b)))
	    s[7 - b] = '-';
    s[8] = 0;
}



#asm
	public	_mob

_mob:	move.b	d0,(a3)+
	rts
#endasm



void formdate(str s, struct DateStamp *when, short space)
{
    char temp[20];
    struct DateTime dt;

    dt.dat_Stamp = *when;
    dt.dat_Format = FORMAT_DOS;		/* FORMAT_DEF ? */
    dt.dat_Flags = cweeek ? DTF_SUBST : 0;
    dt.dat_StrDay = null;
    dt.dat_StrDate = temp;
    dt.dat_StrTime = temp + 10;		/* adjust this if FORMAT_other */
    DateToStr(&dt);
    temp[9] = ' ';
    temp[18] = 0;
    while (space > 18)
	*s++ = ' ', space--;
    strncpy(s, temp, space);
    s[space] = 0;
}



/* Another personal ad found in EXPRESS "The East Bay's Free Weekly":
	HI.
   Yup, that was the whole ad.  Right after it:
	RALPH, a 1967 Cadillac, now accepting devotees.
*/



void Lose(register flip f)
{
    if (f) {
	if (f->comment)
	    FreeMem(f->comment, 80L);
	if (f->slinky && f->softlink > 0)
	    FreeMem(f->slinky, SOFTSIZE);
	FREE(f);
    }
}



void CCch(void)
{
    if (abort < 5 && (SetSignal(0L, SIGMASK) & SIGMASK)) {
	abort = 5;
	StopAllOutput();
	Write(Output(), "\n *** BREAK\n", 12L);
	me->pr_Result2 = 0;
    }
}



void PFault(str head)
{
    pflush();
    if (hair == ERROR_TOO_MANY_LEVELS)
	putfmt("%s: not enough stack space.\n", head);
    else
	PrintFault(hair, head);
}



flip /* flop & */ Fly(register struct FileInfoBlock *b)
{
    register flip z;
    register short tt = b->fib_DirEntryType;

    if (!NEW(z))
	return null;
    z->comment = null;			/* if alloc fails, no big deal */
    if (*b->fib_Comment && (z->comment = Alloc(80)))
	strcpy(z->comment, b->fib_Comment);
    z->next = null;
    z->length = b->fib_Size;
    z->blox = b->fib_NumBlocks;
    z->tection = b->fib_Protection;
    z->when = b->fib_Date;
    z->keee = b->fib_DiskKey;
    strcpy(z->name, b->fib_FileName);
    z->fflags = (tt == 4 || tt == -4 ? F_HARDLINK : 0);
    z->softlink = tt == 3;
    z->dirred = tt > 0;
    z->wanted = z->jected = z->infoed = false;
    z->slinky = null;
    return z;
}



void DoImmediate(flip m, bool immediacy)
{
    if (!m->jected) {
	if (xform)
	    FloorMat(m, true);
	if (immediacy)
	    if (oform && !ctifle)
		FloorMat(m, false);
	    else {
		short fakecol = 0;
		Cough1(m, m->dirred, &fakecol);
	    }
    }
}



flip Scan1(short *ficou, struct FileInfoBlock *deef, BPTR deer, bool noparent)
{
    flip result = null;
    BPTR dp, ocd, fo;
    char fone[31];

    if (!(result = Fly(deef)))
	return null;
    *ficou = 1;			/* we're seeing if it has a .info file */
    dp = ParentDir(deer);
    if ((!cons || !colorful) && strlen(deef->fib_FileName) < 26 && dp) {
	ocd = CurrentDir(dp);
	strcpy(fone, result->name);
	strcat(fone, ".info");
	CCch();
	if (!abort && (fo = RLock(fone))) {
	    result->infoed = true;
	    UnLock(fo);
	}
	CurrentDir(ocd);
    }
    /* WE SEEM to be running into an undocumented feature here...  the
    /* ding bling ParentDir function sometimes sets IoErr = 212 after a
    /* perfectly normal and SUCCESSFUL call.  When it does this, the
    /* following RLock also does so.  So we just band-aid it: */
    if (me->pr_Result2 == ERROR_OBJECT_NOT_FOUND
			|| me->pr_Result2 == ERROR_OBJECT_WRONG_TYPE)
	me->pr_Result2 = 0;
    if (dp)
	UnLock(dp);
    else {
	result->fflags |= F_ROOT;
	result->length = result->tection = 0;		/* invalid bits */
    }
    if (noparent) {
	zurse = zutfils = zutdirs = false;
	if (!oform && !zortless)
	    zomplete = true;
	notadirbutokay = true;
	if (patty)
	    result->jected = notadir = true;
    }
    DoImmediate(result, zortless);
    return result;
}



bool OutOfDate(flip f)
{
    long a;
    struct DateStamp d;

    if (!(before | after))
	return false;
    if (!tooday) {
	DateStamp((adr) &d);
	tooday = d.ds_Days;
    }
    a = 1 + tooday - f->when.ds_Days;
    if (a <= 0) a = 1;
    if (before > after)
	return a > after && a <= before;
    else
	return a > after || a <= before;
}



void Descendify(flip f, BPTR deer, struct Ants *pan, struct cuont *great)
{
    BPTR ocd = CurrentDir(deer), innerdeer;
    if (innerdeer = RLock(f->name)) {
	register bool p = patty;
	if (!canydepth)
	    patty = false;
	rdepth++;
	DoInner(f->name, innerdeer, true, pan, great);
	rdepth--;
	patty = p;
	UnLock(innerdeer);
    } else {
	hair = me->pr_Result2;
	putfmt(" *** Can't lock inner directory \"%s\"!\n", f->name);
    }
    CurrentDir(ocd);
}



flip ScanInside(short *ficou, struct FileInfoBlock *deef, BPTR deer,
		struct Ants *pan, struct cuont *great, adr *potholderptr)
{
    flip result = null, more;
    long air = 0;
    bool prejected, plork;
    /* This is for avoiding explosions with ReadLink: */
    struct DeviceList *vol = gbip(bip(struct FileLock, deer)->fl_Volume);
    bool bogondisk = (vol->dl_DiskType & ~2L) == 0x444F5300;

			/* vvvv prevent multiple "please replace" in -R */
    while (hair != ERROR_DEVICE_NOT_MOUNTED
				&& AsExNext(deer, deef, potholderptr)) {
	CCch();
	if (abort || !(more = Fly(deef))) break;
	prejected = (more->dirred ? zutdirs : zutfils)
			|| (more->tection & protlook) != protwant
			|| OutOfDate(more);
	plork = (!prejected || (curse && more->dirred && !canydepth))
			&& (!patty || MatchPatternNoCase(mesh, more->name));
	more->jected = prejected || !plork;
	if ((plork || canydepth) && zurse && more->dirred && !more->softlink) {
	    more->fflags |= F_DESCEND;
	    if (!cupside)
		Descendify(more, deer, pan, great);
	}
	if (more->softlink && zomplete)
	    if (!bogondisk && (more->slinky = Alloc(SOFTSIZE))) {
		bool r = ReadLink(bip(struct FileLock, deer)->fl_Task,
				  deer, more->name, more->slinky, SOFTSIZE);
		if (!r) {
		    FreeMem(more->slinky, SOFTSIZE);
		    more->slinky = (str) - me->pr_Result2;
		    more->softlink = -1;
		}
	    }
	DoImmediate(more, zortless);
	more->next = result;
	result = more;
	(*ficou)++;
    }
    if (air)
	me->pr_Result2 = air;
    if (me->pr_Result2 == ERROR_NO_MORE_ENTRIES)
	me->pr_Result2 = 0;
    return result;
}



/* scans a Dos directory and returns the findings in a linked list of fly's.
   *ficou tells the number of elements in the list. */

flip ScanDeer(BPTR deer, short *ficou, bool hasparent,
			struct Ants *pan, struct cuont *great)
{
    flip result = null, more;
    register struct FileInfoBlock *deef;
    adr potholder;

    *ficou = 0;
    if (!NEWP(deef)) {
	me->pr_Result2 = ERROR_NO_FREE_STORE;
	return null;
    }
    if (AsExamine(deer, deef, &potholder)) {
	CCch();
	if (!abort)
	    if (deef->fib_DirEntryType < 0 || (csternal
			    && !patty && !zurse && !consumption))
		result = Scan1(ficou, deef, deer, !hasparent);
	    else
		result = ScanInside(ficou, deef, deer, pan, great, &potholder);
    }
#ifdef DEBUG
    if (me->pr_Result2 == -1)
        me->pr_Result2 = 0;		/* work around SDB shortcoming */
#endif
    if (me->pr_Result2 && !abort) {
	hair = me->pr_Result2;
	if (!ctifle)
	    PFault("DR COULDN'T FINISH SCAN");
	CCch();
    }
    AsExCleanup(&potholder);
    FREE(deef);
    if (abort)
	while (result) {
	    more = result;
	    result = result->next;
	    Lose(more);
	}
    return result;
}


void inatl_strupr(register ustr pp)
{
    register ubyte c;
    for ( ; c = *pp; pp++)
	if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c != 0xF7 && c != 0xFF))
	    *pp = c - 32;
}



#ifdef C_NOT_ASM		/* redo these in assembly for speed */

short alpha(ubyte *a, ubyte *b)
{
    register ubyte ac, bc;
    short res, lean = 0;
    do {
	if (!lean)
	    lean = (short) *a - (short) *b;
	ac = sorder[*(a++)];
	bc = sorder[*(b++)];
    } while (ac && ac == bc);
    res = (short) ac - (short) bc;
    return res ? res : lean;
}



short alpo(flip a, flip b)
{
    return alpha(a->name, b->name);
}



short olda(flip a, flip b)
{
    register long t;
    if (t = a->when.ds_Days - b->when.ds_Days)
	return t;
    if (t = a->when.ds_Minute - b->when.ds_Minute)
	return t;
    return a->when.ds_Tick - b->when.ds_Tick;
}



short siez(flip a, flip b)
{
    if (a->length < b->length)
	return -1;
    else
	return a->length > b->length;
}



short infoo(ubyte *a, ubyte *b)
/* returns 0 if filename b is the .info of filename a, positive if b is
   alphabetically after a's .info name, negative if before. */
{
    ubyte acat[36];
    register ubyte ac, bc;
    register short f = (short) sorder[b] - (short) sorder[a];
    if (f)
	return f;
    strcpy(acat, a);
    strcat(acat, ".info");
    a = acat;
    do {
	ac = sorder[*(a++)];
	bc = sorder[*(b++)];
    } while (ac && ac == bc);
    return (short) bc - (short) ac;
}

#else

#pragma regcall(alpo(a0, a1))
#pragma regcall(olda(a0, a1))
#pragma regcall(siez(a0, a1))
#pragma regcall(alpha(a0, a1))
#pragma regcall(infoo(a1, a0))
		/*    ^^  ^^    backwards on purpose */

/* these optimized versions make a noticeable difference when sorting a
directory with 75 files or more */

#asm
	public		_alpha
	public		_alpo
	public		_olda
	public		_siez
	public		_infoo


_alpo:	lea		38(a0),a0		; first->name
	lea		38(a1),a1		; second->name
	; fall thru:

_alpha:	movem.l		a2/d2,-(sp)
	lea		_sorder,a2
	moveq		#0,d1
	moveq		#0,d2
nxt:	  moveq		#0,d0
	  move.b	(a0),d0
	  swap		d0
	  move.b	(a0)+,d0		; get two copies
	  move.b	(a1)+,d1
	  beq		out			; end of string
	  sub.w		d1,d0
	  beq		nxt			; exactly equal chars
	  tst.w		d2
	  bne		leanin			; diff of unsordered bytes
	    move.w	d0,d2			; if no such already found
leanin:	  swap		d0			; restore original byte
	  move.b	(a2,d0.w),d0		; convert by sorder
	  move.b	(a2,d1.w),d1
	  cmp.b		d0,d1
	  beq		nxt			; equal after sorder conversion
out:	sub.w		d1,d0			; compare as unsigned bytes
	bne		unlean
	  move.w	d2,d0			; use lean if otherwise equal
unlean:	movem.l		(sp)+,a2/d2
	rts


_olda:	move.l		16(a0),d0		; a->when.ds_Days
	sub.l		16(a1),d0		; - b->when.ds_Days
	bne		zulte
	move.l		20(a0),d0		; a->when.ds_Minute
	sub.l		20(a1),d0		; - b->when.ds_Minute
	bne		zulte
	move.l		24(a0),d0		; a->when.ds_Tick
	sub.l		24(a1),d0		; - b->when.ds_Tick
zulte:	rts


_siez:	move.l		4(a0),d0		; a->length
	sub.l		4(a1),d0		; - b->length
	bge		nnegrt
	  moveq		#-1,d0
	  rts
nnegrt:	bne		posrt
	moveq		#0,d0
	rts
posrt:	moveq		#1,d0
	rts


_infoo:	move.l		a2,-(sp)
	moveq		#0,d0
	moveq		#0,d1
	move.b		(a0),d0			; "b"
	move.b		(a1),d1			; "a"
	lea		_sorder,a2
	move.b		0(a2,d0.w),d0
	move.b		0(a2,d1.w),d1
	sub.b		d1,d0			; quick pre-test
	beq		check
	  ext.w		d0			; return test diff if nonzero
	  move.l	(sp)+,a2
	  rts					
check:	link		a5,#-40			; the real test
	move.l		sp,a2			; temporary copy area
cpy:	  move.b	(a1)+,(a2)+
	  bne		cpy
	move.b		#'.',-1(a2)		; overwrite final nul
	move.b		#'i',(a2)+
	move.b		#'n',(a2)+
	move.b		#'f',(a2)+
	move.b		#'o',(a2)+
	clr.b		(a2)
	move.l		sp,a1
	lea		_sorder,a2
loeup:	  move.b	(a0)+,d0
	  move.b	(a1)+,d1
	  beq		pueol
	  move.b	(a2,d0.w),d0
	  move.b	(a2,d1.w),d1
	  cmp.b		d1,d0
	  beq		loeup
pueol:	sub.w		d1,d0
	unlk		a5
	move.l		(sp)+,a2
	rts

#endasm

import short alpo(flip a, flip b), olda(flip a, flip b), siez(flip a, flip b);
import short alpha(ubyte *a, ubyte *b);
import short infoo(ubyte *a, ubyte *b);

#endif



#ifdef C_NOT_ASM

flip MergeSort(flip flist, short ficou, bool reverse,
		short (*sortie)(flip a, flip b), short (*second)(flip a, flip b))
{
    register flip l, l1, l2;
    flip *p;
    register short i, oint, s;

    if (ficou <= 2) {				/* handle degenerate cases */
	if (ficou < 2) return flist;
	l = flist->next;
	if (!(s = sortie(flist, l)))
	    s = second(flist, l);
	if ((s > 0) ^ reverse) {
	    flist->next = null;
	    l->next = flist;
	    flist = l;
	}
	return flist;
    }
    oint = ficou >> 1;				/* split list in two */
    l1 = l = flist;
    for (i = 1; i < oint; i++)
	l = l->next;
    l2 = l->next;
    l->next = null;
    l1 = MergeSort(l1, oint, reverse, sortie, second);    /* sort each half */
    l2 = MergeSort(l2, ficou - oint, reverse, sortie, second);
    p = &flist;						/* merge sublists */
    while (l1 && l2) {
	if (!(s = sortie(l1, l2)))
	    s = second(l1, l2);
	if ((s < 0) ^ reverse)
	    l = l1, l1 = l1->next;
	else
	    l = l2, l2 = l2->next;
	*p = l;
	p = &l->next;
    }
    if (l1)
	*p = l1;
    else
	*p = l2;
    return flist;
}

#else

#asm
		public		_MergeSort

regzz		reg		d2-d7/a2-a6

_MergeSort:	movem.l		regzz,-(sp)
		move.l		a0,a5		; flist
		move.l		a1,a6		; sortie
		move.l		a2,d5		; second
		cmp.w		#2,d2		; ficou
		bgt		split
		  bne		retflist	; list length 1, return it
		  move.l	(a5),a2		; length 2, check order
		  move.l	a2,a1		; ^^ l = flist->next
		  move.l	a5,a0
		  jsr		(a6)		; s = sortie(flist, l)
		  tst.w		d0
		  bne		nosecond	; no difference?
		    move.l	a2,a1
		    move.l	a5,a0
		    exg		d5,a6
		    jsr		(a6)		; s = second(flist, l)
		    exg		a6,d5
nosecond:	  tst.w		d6		; if (reverse) s = -s
		  beq		noneg1
		    neg.w	d0
noneg1:		  tst.w		d0
		  ble		retflist	; is in correct order
		  clr.l		(a5)		; flist->next = null
		  move.l	a5,(a2)		; l->next = flist
		  move.l	a2,a5		; flist = l
		  bra		retflist

split:		move.w		d2,d4		; save for later
		move.w		d2,d3
		asr.w		#1,d3		; oint = ficou / 2
		move.l		a5,a2		; l = flist
		move.l		a5,a3		; l1 = flist
		move.w		d3,d0
		subq		#1,d0
zoop:		  dbra		d0,zinn		; do oint-1 times
		    bra		zun		; yes we can go through 0 times
zinn:		  move.l	(a2),a2		; l = l->next
		  bra		zoop
zun:		move.l		(a2),d7		; l2 = l->next
		clr.l		(a2)		; l->next = null

		move.l		a2,-(sp)	; save
		move.l		d5,a2		; second
		move.l		a6,a1		; sortie
		move.w		d3,d2		; oint
		move.l		a3,a0		; l1
		bsr		_MergeSort	; l1 = MergeSort(l1, oint,
		move.l		d0,a3		;        reverse, sortie, second)
		move.l		a6,a1		; sortie again
		move.w		d4,d2
		sub.w		d3,d2		; ficou - oint
		move.l		d7,a0		; l2
		bsr		_MergeSort	; l2 = MergeSort(l2, ficou - oint,
		move.l		d0,d7		;        reverse, sortie, second)
		move.l		(sp)+,a2	; restore

		move.l		a5,-(sp)	; put flist in memory
		move.l		sp,a5		; p = &flist
elihu:		  move.l	a3,d0		; pseudo tst.l
		  beq		finnish
		  move.l	d7,d0
		  beq		finnish		; neither list empty?
		  move.l	d7,a1
		  move.l	a3,a0
		  jsr		(a6)		; s = sortie(l1, l2)
		  tst.w		d0
		  bne		nosecond2
		    move.l	d7,a1
		    move.l	a3,a0
		    exg		d5,a6
		    jsr		(a6)		; s = second(l1, l2)
		    exg		a6,d5
nosecond2:	  tst.w		d6
		  beq		noneg2
		    neg.w	d0
noneg2:		  tst.w		d0
		  bge		twooo
		    move.l	a3,a2		; l = l1
		    move.l	(a3),a3		; l1 = l1->next
		  bra		onnne
twooo:		    move.l	d7,a2		; l = l2
		    exg		a0,d7
		    move.l	(a0),a0		; l2 = l2->next
		    exg		a0,d7
onnne:		  move.l	a2,(a5)		; *p = l
		  move.l	a2,a5		; p = &l->next
		  bra		elihu
finnish:	move.l		a3,d0		; is l1 empty?
		beq		tuuue
		  move.l	a3,(a5)		; *p = l1
		bra		retstarp
tuuue:		  move.l	d7,(a5)		; *p = l2
retstarp:	move.l		(sp)+,a5

retflist:	move.l		a5,d0
		movem.l		(sp)+,regzz
		rts

#endasm

import flip MergeSort(flip flist, short ficou, bool reverse,
		short (*sortie)(flip a, flip b), short (*second)(flip a, flip b));

#pragma regcall(MergeSort(a0, d2, d6, a1, a2))

#endif



flip Soart(flip flist, short ficou)
{
    register flip t, tt;
    register short k;

    if (!flist)
	return null;
    if (!cons) {
/* alpha sorting greatly speeds the following .info rejector, enough to
justify doing an alpha sort and then doing a different sort afterwards: */
	flist = MergeSort(flist, ficou, false, alpo, siez);
	for (t = flist; t; t = t->next)
	    for (tt = t->next; tt; tt = tt->next)
		if (!(t->jected | tt->dirred))
		    if (!(k = infoo((ubyte *) t->name, (ubyte *) tt->name))) {
			t->infoed = tt->jected = true;
			break;
		    } else if (k > 0)
			break;
    }
    if (cizesort)
	flist = MergeSort(flist, ficou, creverse, siez, (cron ? olda : alpo));
    else if (cron)
	flist = MergeSort(flist, ficou, creverse, olda, alpo);
    else if (!cons || creverse)
	flist = MergeSort(flist, ficou, creverse, alpo, siez);
    return flist;
}



short CheckWindowWidth(void)
{
    struct InfoData *ind;
    adr cont = bip(struct FileHandle, me->pr_COS)->fh_Type;

    if (!cuca)
	if (color && cont && NEWP(ind)) {
	    if (DoPkt1(cont, (long) ACTION_DISK_INFO, (ulong) ind >> 2))
		cuca = (adr) ((struct IOStdReq *) ind->id_InUse)->io_Unit;
	    else color = false;
	    FREE(ind);
	} else color = false;
    return color ? cuca->cu_XMax + 2 : WIDEFAULT + 1;
}
/* I know, there's an escape sequence.  But it don't work without you does
set_raw, which does dos_packet, and it comes out smaller this way.  I could
probably save 100 bytes or so under 1.3 by doing my own packet handling ... */

/* we actually return the number of characters that will fit plus one.  This is
because most calculations relating to window fit have to subtract one to
compensate for the lack of a one space pad after the last column.  So it saves
some arithmetic to add the one first. */



void Columnate(flip flist, short ficou)
{
    short n, c = 0;
    bool filongest = false;
    register flip f;
/*  bool oneinfo = false;	FLACK */

    song = keygits = 1;
    for (f = flist; f; f = f->next)
	if (!f->jected) {
	    c++;
	    if ((n = digits(f->length)) > song)
		song = n;
	    if (ceys && (n = digits(f->keee)) > keygits)
		keygits = n;
#ifdef MARKLINKS
	    if ((f->fflags & F_HARDLINK) || f->softlink)
		anylinks = true;
#endif
	    if (!f->dirred)
		anyfiles = true;
	}
    if (consumption || oform || zortless)
	return;
    cwid = 1;
    if (!wid)
	wid = CheckWindowWidth();
    for (f = flist; f; f = f->next)
	if (!f->jected) {
	    n = strlen(f->name) + 1 /* 2 - cons */ ;		/* FLACK */
	    if (f->dirred && !f->softlink)
		n++;
	    if (zize & anyfiles)
		n += song + 1;
	    if (ceys)
		n += keygits + 3;
	    if (n >= cwid) {
		if (!steil && !f->dirred || n > cwid)
		    filongest = !f->dirred;
		cwid = n;
	    }
/*	    if (f->infoed)
		oneinfo = true;	FLACK */
	}
/*  zons = cons;
    if (!(cons | oneinfo)) {
	zons = true;
	if (!--cwid) cwid = 1;
    }				FLACK */
    if (!zomplete) {
	short kolz;
	if (!steil) {			/* dented */
	    if (zize)
		n = (short) !filongest;
	    else
		n = - (short) filongest;
	    cols = (wid + n) / cwid;
	} else
	    cols = wid / cwid;
	if (cols > MAXCOLS) cols = MAXCOLS;
/*	if (cols > ficou) cols = ficou; */ /* on second thought, nah */
	if (!cols) cols = 1;		 /* window too narrow */
	kolz = (cols > c ? cols : c);
/***	cwid = wid / cols; **/		 /* share extra space evenly? NAH */
	if (wid / kolz > cwid)		 /* add at most 1 space */
	    cwid++;
#ifdef OLD_LWID_STUFF	/* stretch to use whole window width */
    } else {
	n = wid - /*** (ceys ? 31 + keygits : 28) ***/ 28;
	if (cwid > n)
	    cwid = n;			/* not enough space */
	if (n > LWID)
	    n = LWID;			/* use at least LWID if possible */
	if (n > cwid)
	    cwid = n;
#endif
    }
}



#ifdef SOMEDAY_MAYBE

here are some rough notes for an algorithm for doing ultra-compact columns.
note that this does not take into account the separation of files from
directories, or other such details.

declared in g:  short ciwd[MAXCOLS], song[MAXCOLS], keygits[MAXCOLS];

call columnate in a loop like this:
    for (cols = MAXCOLS; !Columnate(flist, ficou; cols--) ;

bool Columnate(flip flist, short ficou)
{
    short rose = (ficou + cols - 1) / cols, jag = ficou % cols;
    short col, row, ord, clot = 0;
    ... plus most of the variables it already has

    if (!jag) jag = cols;
    for (col = 0; col < cols; col++) {
	if (colsort) {
	    ord = col * rose;
	    if (col > jag) ord -= col - jag;
	} else ord = col;
	for (row = 0; ord < ficou; row++) {
	    y = ... the ord'th member of flist
	    ... measure length of name etc. as above, but assign results to
	    ... cwid[col] and song[col] and keygits[col]
	    y->ordination = ord;
	    if (colsort) ord++;
	    else ord += cols;
	}
	clot += cwid[col];		/* total page width used so far */
	if (clot > wid && cols > 1)
	    return false;			/* fail, try fewer cols */
    }
    return true;
}

#endif



str Joyn(str bp, str bs, str add)
{
    size_t l = strlen(add);
    if (l > 255 - (bp - bs))
	return null;
    strcpy(bp, add);
    bp += l;
    return bp;
}



void FloorMat(flip y, bool kseeq)
{
    char bluff[258], c;
    ubyte cb;
    short i;
    long r, narg;
    str bb = &bluff[1], bp, form = (kseeq ? xform : oform), tp, ttp;
    bool noret = false, narged;
    bool collor = color & colorful & y->infoed && !kseeq && !zortless;
    static long systagses[3] = { SYS_UserShell, TRUE, TAG_DONE };
    static str weekdaynames[7] = { "Sunday   ", "Monday   ", "Tuesday  ",
			"Wednesday", "Thursday", "Friday   ", "Saturday " };
    void mob(char c, str b);

    if (kseeq & ctifle)
	return;
    bluff[0] = ' ';		/* so bp[-1] always sees something */
    if (y->fflags & F_ROOT)
	prepath[0] = '\0';
    for (bp = bb; *form && bp - bb < 256; form++)
	if (*form == '\\') {
	    narg = 0, narged = false;
	    while (isdigit(c = *++form))
		narg = narg * 10 + c - '0', narged = true;
	    switch (lower(c)) {
		case 'n':
		    *(bp++) = '\n';
		    break;
		case 'e':
		    *(bp++) = 27;
		    break;
		case '+':
		    noret = true;
		    break;
		case '/':
		    cb = bp[-1];
		    if (cb != '/' && cb != ':' && cb != '"' && cb > ' ')
			*(bp++) = '/';
		    break;
		case '?':
		    if ((cb = bp[-1]) != '/' && cb != ':')
			if (y->fflags & F_ROOT)
			    *(bp++) = ':';
			else if (y->dirred && !y->softlink)
			    *(bp++) = '/';
		    break;
		case 'i':
		    if (!narged)
			narg = 1;
		    narg *= rdepth;
		    if (bp - bb > 255 - narg)
			goto aarrgh;
		    for (i = 0; i < narg; i++)
			*(bp++) = ' ';
		    break;
		case 't':
		    if (!narged)
			narg = 18;
		    if (bp - bb >= 256 - narg)
			goto aarrgh;
		    formdate(bp, &y->when, narg);
		    bp += narg;
		    break;
		case 'w':
		    if (!narged)
			narg = 9;
		    if (bp - bb >= 256 - narg)
			goto aarrgh;
		    while (narg > 9)
			*bp++ = ' ', narg--;
		    strncpy(bp, weekdaynames[y->when.ds_Days % 7], narg);
		    bp += narg;
		    *bp = 0;
		    break;
		case 'b':
		    if (bp - bb >= 248)
			goto aarrgh;
		    fortection(bp, y->tection);
		    bp += 8;
		    break;
		case 's':
		    i = (y->dirred ? 0 : digits(y->length));
		    if (!narged)
			narg = 9;
		    if (i > narg)
			narg = i;
		    if (bp - bb >= 256 - narg)
			goto aarrgh;
		    memset(bp, ' ', narg - i);
		    if (!y->dirred)
			RawDoFmt("%ld", &y->length, mob, bp + narg - i);
		    bp += narg;
		    break;
		case 'k':
		    i = digits(y->keee);
		    if (!narged)
			narg = 6;
		    if (i > narg)
			narg = i;
		    if (bp - bb >= 256 - narg)
			goto aarrgh;
		    memset(bp, ' ', narg - i);
		    RawDoFmt("%ld", &y->keee, mob, bp + narg - i);
		    bp += narg;
		    break;
		case 'd':
		    tp = prepath + strlen(prepath) - 1;
		    if (notadirbutokay) {
			ttp = tp;
			if (!*prepath || (*tp == '/' &&
					    (tp[-1] == '/' || tp < prepath)))
			    tp++;
			else
			    while (--tp >= prepath)
				if (*tp == '/' || *tp == ':')
				    goto flarp;
			    tp = ttp;
		    }
		  flarp:
		    c = *++tp;
		    *tp = 0;
		    if (!(bp = Joyn(bp, bb, prepath))) {
			*tp = c;
			goto aarrgh;
		    }
		    *tp = c;
		    break;
		case 'f':
		    if (collor && !(bp = Joyn(bp, bb, CSI "33m")))
			goto aarrgh;
		    if (!(bp = Joyn(bp, bb, y->name)))
			goto aarrgh;
		    if (collor && !(bp = Joyn(bp, bb, CSI "31m")))
			goto aarrgh;
		    break;
		case 'p':
		    if (collor && !(bp = Joyn(bp, bb, CSI "33m")))
			goto aarrgh;
		    if (!(bp = Joyn(bp, bb, prepath)))
			goto aarrgh;
		    if (y->fflags & F_ROOT || !*prepath || (!notadirbutokay &&
					(!csternal || zurse || consumption))) {
			if (*prepath && bp[-1] != '/' && bp[-1] != ':')
			    *(bp++) = '/';
			if (!(bp = Joyn(bp, bb, y->name)))
			    goto aarrgh;
		    }
		    if (collor && !(bp = Joyn(bp, bb, CSI "31m")))
			goto aarrgh;
		    break;
		default:
		    *(bp++) = *form;
	    }
	} else
	    *(bp++) = *form;
  aarrgh:
    *bp = 0;
    CCch();
    if (*form) {
	putfmt("\n *** Line produced by \"%s\" too long!\n", y->name);
	if (!hair) hair = ERROR_LINE_TOO_LONG;
    } else if (!abort) {
	if (kseeq) {
	    pflush();
	    r = System(bb, (adr) systagses);
	    if (r >= 10) {
		putfmt(
"\n *** QUITTING; Command returned %ld, error code %ld.\n", r, me->pr_Result2);
		abort = 10;
	    }
	} else {
	    put(bb);
	    if (!noret) {
		puch('\n');
		CCch();
	    } else if (*bb)
		needsnl = true;
	}
    }
}



void Cough1(register flip y, bool dirs, short *col)
{
    register short lused = strlen(y->name);
    bool icon = color & colorful & y->infoed;
    char p, buf1[10], buf2[33];
#ifdef DEBUG
    if (!stricmp(y->name, "moned.readme"))
	CCch();
#endif

    if (ctifle) {
	abort = 1;	/* we found one, we found one! */
	return;
    }
    if (zortless) {
	song = 9;	/* extreme cases could overflow these */
	keygits = 6;
#ifdef MARKLINKS
	anylinks = true;
#endif
	anyfiles = true;
    }
    if (consumption && !cortless)
	return;
    if (anyfiles && zize) {
	if (y->dirred)
	    pad(song);
	else
	    padong(y->length, song);
	if (zomplete || !y->dirred || *col || steil)
	    puch(' ');
	lused += song + 1;
    }
    if (!steil && !y->dirred && !*col && !zize && !zortless)
	puch(' ');			/* dent */
    if (ceys && (zomplete || !zortless)) {
	puch('[');
	padong(y->keee, keygits);
	put("] ");
	lused += 3 + keygits;
    }
/*  if (!zons) {
	puch(y->infoed ? FLACK : ' ');
	lused++;
    } */
    if (zomplete) {
	fortection(buf1, y->tection);
	formdate(buf2, &y->when, 18);
#ifdef MARKLINKS
	putfmt("%s %s ", buf1, buf2);
	if (anylinks) {
	    if (y->fflags & F_HARDLINK)
		put("H> ");
	    else if (y->softlink)
		put("S> ");
	    else
		put("   ");
	} else
	    puch(' ');
#else
	putfmt("%s %s  ", buf1, buf2);
#endif
    }
    if (icon)
	put(CSI "33m");
    if (zortless) {
	put(prepath);
	if (*prepath) {
	    register short l = strlen(prepath);
	    p = prepath[l - 1];
	    if (p != '/' && p != ':')
		puch('/');
	}
    }
    put(y->name);
    if (icon)
	put(CSI "31m");
    if (y->dirred && !y->softlink) {
	puch(y->fflags & F_ROOT ? ':' : '/');
	lused++;
    }
    if (zomplete) {
	puch('\n');
	if (y->softlink > 0 && y->slinky)
	    putfmt(" >>> soft link to \"%s\"\n", y->slinky);
	else if (y->softlink < 0) {
	    pflush();
	    PrintFault(-(long) y->slinky, " *** Couldn't read soft link");
	}
	if (y->comment)
	    putfmt(": %s\n", y->comment);
    } else if (zortless)
	puch('\n');
    else {
	if (++*col >= cols) {
	    *col = 0;
	    puch('\n');
	} else
	    pad(cwid - lused);
    }
}



bool CoughHalf(flip flist, bool dirs)
{
    short h, col = 0, n = 0;
    bool anyleft = false;
    register flip y;

    if (!flist || zortless)
	return false;
    for (y = flist; y; y = y->next)
	if (!y->jected)
	    if (steil == 3 || !(dirs ^ y->dirred)) {
		y->wanted = true;
		n++;
	    } else {
		y->wanted = false;
		anyleft = true;
	    }
	/* else wanted is always false */
    if (!n)
	return false;
    h = 0;
    for (y = flist; y; y = y->next)
	if (y->wanted) {
	    y->ordination = h;
	    if (colsort) {
		if ((h += cols) >= n)
		    h = h % cols + 1;
	    } else h++;
	} else
	    y->ordination = -1;
    y = flist;
    for (h = 0; h < n; h++) {
	while (y->ordination != h)
	    if (!(y = y->next))
		y = flist;		/* rather inefficient... */
	CCch();
	if (abort) break;
	if (oform)
	    FloorMat(y, false);
	else
	    Cough1(y, dirs, &col);
    }
    if (col > 0)
	puch('\n');
    return anyleft && !abort && !zutbot;
}



/* short */ void plural(str s, long n1, long n2 /* , short diffs */ )
{
    str ss = (n1 == 1 && !n2 ? "" : "s");
    if (!n2)
	putfmt("%ld ", n1);
    else {
/**	diffs++;					**/
	n2 += n1;
/** 	if (diffs > 1)					**/
	    putfmt("%ld (%ld) ", n1, n2);
/**	else putfmt("%ld (of %ld) ", n1, n2);		**/
    }
    putfmt(s, ss);
/** return diffs;					**/
}



void Tote(register struct cuont *c)
{
/** short diffs = 0;					**/
    if (!c[1].dir && !c[1].fil && c[0].dir + c[0].fil <= 1) {
	if (!c[0].dir && !c[0].fil) return;		/**** I DUNNO... */
	putfmt("1 %s, ", c[0].dir ? "dir" : "file");
	plural("block%s.\n", c[0].blok, 0L /* , 0 */ );
    } else {
	/* diffs = */ plural("dir%s, ", c[0].dir, c[1].dir /* , diffs */ );
	/* diffs = */ plural("file%s, ", c[0].fil, c[1].fil /* , diffs */ );
	/* diffs = */ plural("byte%s, ", c[0].byt, c[1].byt /* , diffs */ );
	plural("block%s.\n", c[0].blok, c[1].blok /* , diffs */ );
    }
}



void Header(short ficou, str name)
{
    str nn = *name ? name : "(current dir)";
    CCch();
    if (abort)
	return;
    if (didaninny && (!consumption || oform || cortless))
	puch('\n');
    if (ficou)
	putfmt("        ===  %s  ===\n", nn);
    else
	putfmt("        ===  %s is empty.  ===\n", nn);
    didaninny = true;
}



void CoughUp(flip flist, short ficou)
{
    cols = 1;		/* default */
#ifdef MARKLINKS
    anylinks = false;
#endif
    anyfiles = false;
    Columnate(flist, ficou);
    if (cupside) {
	zuttop = zutfils;
	zutbot = zutdirs;
    } else {
	zuttop = zutdirs;
	zutbot = zutfils;
    }
    if (!zuttop && CoughHalf(flist, !cupside) && !consumption)
	if (steil == 2) {
	    short i;
	    put("    ");
	    for (i = 0; i < wid - 10; i++)
		puch('-');
	    puch('\n');
	} else if (steil == 1)
	    puch('\n');
    CCch();
    if (steil != 3 && !zutbot && !abort)
	CoughHalf(flist, cupside);
}



void Tad(register struct cuont *des, register struct cuont *src)
{
    register short i;
    for (i = 0; i < 2; i++) {
	des->blok += src->blok;
	des->byt += src->byt;
	des->fil += src->fil;
	des->dir += src->dir;
	des++;
	src++;
    }
}



void DoInner(str what, BPTR deer, bool hasparent,
			struct Ants *ants, struct cuont *great)
{
    flip filist, f;
    bool bigger, heads, viz = false;
    struct cuont tot[2], gran[2];	/* [0] is non-jected, [1] is jected */
    struct Ants parant, *pa;
    short ficou, prength = strlen(prepath), preng1 = prength;
    char pe = prepath[prength - 1];

    gran[0].blok = gran[0].byt = gran[0].fil = gran[0].dir = 0;
    tot[1] = tot[0] = gran[1] = gran[0];
    parant.prav = ants;
    parant.kay = bip(struct FileLock, deer)->fl_Key;
    if (!abort && StackLeft() < STACKNEEDED) {
	putfmt("*** Cannot list \"%s\" -- insufficient stack space!\n", what);
	hair = ERROR_TOO_MANY_LEVELS;
	return;
    }
    if (prength && pe != ':' && pe != '/') {
	prepath[prength++] = '/';
	prepath[prength] = 0;
    }
    zurse = curse | canydepth;
    zortless = ctifle | cortless;
    zize = (complete || (cize && !zortless)) && !oform;
    zomplete = complete && !oform;
    zutfils = cutfils;
    zutdirs = cutdirs;
    if (hasparent || !complete || oform || !zortless) {
	if (prength < PREPENGTH - strlen(what)) {
	    strcpy(prepath + prength, what);
	    if (!hasparent && csternal)
		if (prepath[strlen(prepath) - 1] == ':')
		    prepath[0] = 0;
		else
		    *PathPart(prepath) = 0;
	}
    }
    for (pa = ants; pa; pa = pa->prav)
	if (pa->kay == parant.kay) {
	    putfmt(" *** %s is its own ancestor, through a link!\n\n", prepath);
	    /* don't set hair */
	    goto skipit;
	}
    heads = (didaninny | morethan1) || hasparent || (cupside & zurse);
    if (heads && zortless && (zize | consumption) && !hasparent && !ctifle)
	Header(1, what);
    filist = ScanDeer(deer, &ficou, hasparent, &parant, gran);
    if (!filist && !abort && (me->pr_Result2 == ERROR_NO_FREE_STORE)) {
	putfmt("Not enough memory to list \"%s\"!\n", what);
	hair = ERROR_NO_FREE_STORE;
    }
    zize |= zomplete;
    if (!(consumption && !oform) && !cortless && !ctifle)
	filist = Soart(filist, ficou);
    for (f = filist; f; f = f->next) {
	register struct cuont *tt = tot + f->jected;
	if (f->dirred)
	    tt->dir++;
	else {
	    tt->byt += f->length;
	    tt->fil++;
	}
	tt->blok += f->blox + 1;
	if (!f->jected && !(f->dirred ? zutdirs : zutfils))
	    viz = true;
    }
    heads |= didaninny;
    if (heads && viz && !zortless)
	Header(ficou, prepath);
    if (!abort)
	CoughUp(filist, ficou);
    if ((consumption | zize) && !abort && !cortless && !ctifle) {
	if (notadir) {
	    putfmt("\"%s\" is a file, not a directory.\n", prepath);
	    hair = ERROR_OBJECT_WRONG_TYPE;
	} else
	    Tote(tot);
    }
    CCch();
    if (cupside)
	for (f = filist; f; f = f->next)
	    if (f->fflags & F_DESCEND)
		Descendify(f, deer, &parant, gran);
    Tad(gran, tot);
    if (great)
	Tad(great, gran);
    bigger = gran[0].blok + gran[1].blok > tot[0].blok + tot[1].blok;
    if (!abort && !ctifle && (consumption | zize)
				&& (cortless ? !hasparent : bigger)) {
	if (notadir) {
	    putfmt("\"%s\" is a file, not a directory.\n", prepath);
	    hair = ERROR_OBJECT_WRONG_TYPE;
	} else {
	    if (bigger && !cortless)
		put("Total:  ");
	    Tote(gran);
	}
    }
    while (filist) {
	f = filist->next;
	Lose(filist);
	filist = f;
    }
  skipit:
    prepath[preng1] = 0;		/* remove tail just added */
}



/* SplitTailPat takes dir/pat string in from and translates it into a compiled
   pattern in mesh (and uncompiled in pat) and a lock on the directory in deer
   to be scanned using the pattern, or if deer is also a pattern, a struct
   AnchorPath in ankh. */

bool SplitTailPat(str from, BPTR *deer)
{
    ubyte pat[PATLIMIT + 1];
    short w;
    register ustr p = FilePart(from);

    if (strlen(p) > PATLIMIT)
	w = -1;
    else {
	strcpy(pat, p);
	inatl_strupr(pat);
	w = ParsePatternNoCase(pat, mesh, PATLIMIT * 2 + 2L);
    }
    if (w < 0) {
	if (!ctifle)
	    putfmt("Bogus pattern \"%s\".\n", p);
	hair = ERROR_LINE_TOO_LONG;
	return false;
    }
    if (w) {
	*(PathPart(from)) = 0;
	patty = true;
    }
    if (!deer)
	return true;
    if (w)
	*deer = RLock(from);
    if (!*deer) {
	if (ankh = AllocPZ(sizeof(*ankh) + ABSIZE - 1)) {
	    strncpy(abspath, from, ABSIZE);
	    abspath[ABSIZE] = 0;
	    ankh->ap_Strlen = ABSIZE;
	    if (hair = MatchFirst(abspath, ankh)) {
		FreeMem(ankh, sizeof(*ankh) + ABSIZE - 1);
		ankh = null;
	    }
	} else
	    hair = ERROR_NO_FREE_STORE;
	me->pr_Result2 = hair;
	if (hair && !ctifle && hair != ERROR_NO_MORE_ENTRIES) {
	    putfmt("Couldn't list \"%s\"", from);
	    if (hair == ERROR_BAD_TEMPLATE)
		putn(": bogus pattern.");
	    else
		PFault("");
	}
	return !hair;
    }
    return true;
}



bool HandleMultiAssign(str what, str bowel)
{
    struct AssignList *al;
    struct DosList *dl;
    short l, locount = 1;
    BPTR lox[MULTILIMIT];

    if (dl = LockDosList(LDF_ALL | LDF_READ)) {
	*bowel = 0;
	dl = FindDosEntry(dl, what, LDF_ALL | LDF_READ);
	*bowel = ':';
	if (dl && dl->dol_Type == DLT_DIRECTORY
			&& (al = dl->dol_misc.dol_assign.dol_List)) {
	    if (patty && !SplitTailPat(bowel + 1, 0))
		return false;
	    lox[0] = DupLock(dl->dol_Lock);
	    while (al && locount < MULTILIMIT) {
		lox[locount++] = DupLock(al->al_Lock);
		al = al->al_Next;
	    }
	    UnLockDosList(LDF_ALL | LDF_READ);
	    morethan1 = true;
	    for (l = 0; l < locount; l++)
		if (lox[l]) {
		    if (!abort)
			if (NameFromLock(lox[l], abspath, ABSIZE))
			    DoInner(abspath, lox[l], null, null, null);
			else {
			    didaninny = true;
			    pflush();
			    PrintFault(me->pr_Result2,
				     "\n *** skipping one assignment");
			}
		    UnLock(lox[l]);
		} /* else... ?  lox[l] should never be null */
	    return true;
	}
	UnLockDosList(LDF_ALL | LDF_READ);
    }
    return false;
}



void Do(str what)
{
    BPTR deer = -1, ocd;
    short wl = strlen(what);
    register str p = what + wl, pp;

    notadir = notadirbutokay = false;
    if (wl >= 2 && (canydepth = p[-1] == ':' && p[-2] == ':'))
	p[-2] = 0;
    if (csternal && !consumption) {
	char beastspace[260];
	ubyte buf[SOFTSIZE];
	struct DevProc *dp = null;
	str beast = (str) (((ulong) &beastspace[3]) & ~3);

	do {
	    dp = GetDeviceProc(what, dp);
	    if (dp) {
		beast[0] = wl;
		strncpy(beast + 1, what, (size_t) wl);
		if (deer = DoPkt3(dp->dvp_Port, (long) ACTION_LOCATE_OBJECT,
				    dp->dvp_Lock, (long) beast >> 2,
				    (long) ACCESS_READ))
		    break;
		if (me->pr_Result2 == ERROR_IS_SOFT_LINK)
		    if (ctifle) {
			abort = 1;
			return;
		    } else {
			long suck = ReadLink(dp->dvp_Port, dp->dvp_Lock,
						what, buf, SOFTSIZE);
			if (suck)
			    putfmt("%s is a soft link to \"%s\".\n", what, buf);
			else {
			    hair = me->pr_Result2;
			    putfmt("*** soft link %s ", what);
			    PFault("UNREADABLE");
			}
			FreeDeviceProc(dp);
			return;
		    }
	    }
	} while (dp);
	FreeDeviceProc(dp);
    }
    p = strchr(what, ':');
    if (p && (pp = p + 1, !*pp || (!strchr(pp, '/')
			&& (deer == -1 ? !(deer = RLock(what)) : !deer)))) {
	patty = !!*pp;
	if (HandleMultiAssign(what, p)) {
	    if (deer && deer != -1)
		UnLock(deer);
	    return;
	}
    }
    if (deer == -1)
	deer = RLock(what);
    if (!deer)
	if (!SplitTailPat(what, &deer))
	    return;
    if (ankh) {
	morethan1 = true;
	do {
	    if (!patty || ankh->ap_Info.fib_EntryType > 0) {
		ocd = CurrentDir(ankh->ap_Current->an_Lock);
		if (deer = RLock(ankh->ap_Info.fib_FileName)) {
		    *prepath = 0;
		    if (!convertpath ||	!NameFromLock(deer, abspath, ABSIZE))
			strcpy(abspath, ankh->ap_Buf);
		    DoInner(abspath, deer, null, null, null);
		    UnLock(deer);
		} else {
		    hair = me->pr_Result2;
		    putfmt(" *** Can't lock directory \"%s\"!\n", abspath);
		}
		CurrentDir(ocd);
	    }
	    CCch();
	} while (!abort && !MatchNext(ankh));
	MatchEnd(ankh);
	FreeMem(ankh, sizeof(*ankh) + ABSIZE - 1);
	ankh = null;
    } else {
	if (!deer) {
	    if (!ctifle)
		putfmt("Couldn't find \"%s\".\n", what);
	    hair = ERROR_OBJECT_NOT_FOUND;
	    return;
	}
	if (convertpath && NameFromLock(deer, abspath, ABSIZE))
	    what = abspath;
	*prepath = 0;
	DoInner(what, deer, null, null, null);
	UnLock(deer);
    }
}



void Opt(register ustr p)
{
    register ubyte c;
    for (c = lower(*++p) ; c > ' '; c = lower(*++p)) {
	switch (c) {
	    case 'r':  curse ^= true; continue;
	    case 'i':  cons ^= true; continue;
	    case 'm':  colorful ^= true; continue;
	    case 's':  cize ^= true; continue;
	    case 'c':  cron ^= true; continue;
	    case 'l':  complete ^= true; continue;
	    case 'h':  colsort ^= true; continue;
	    case 'f':  cutdirs ^= true; cutfils = false; continue;
	    case 'd':  cutfils ^= true; cutdirs = false; continue;
	    case 'x':  csternal ^= true; continue;
	    case 'o':  cortless ^= true; continue;
	    case 'u':  consumption ^= true; continue;
	    case 'k':  ceys ^= true; continue;
	 /* case 'g':  cage ^= true; continue;      */
	    case 'v':  creverse ^= true; continue;
	    case 'y':  cweeek ^= true; continue;
	    case 'z':  cizesort ^= true; continue;
	    case '!':  cursorhide = true; continue;	/* for EnvOpts */
	    case '?':  continue;			/* likewise */
	    case '-':  continue;			/* likewise */
	    case '`':  continue;			/* lower('@') == '`' */
	    case 't':  convertpath ^= true; continue;
	    case 'p':  {
		register bool tilt;
		register short b;
		protlook = protwant = 0;
		while ((c = lower(p[1])) > ' ') {
		    if (tilt = (c == '~')) {		/* lower('^') == '~' */
			c = lower((++p)[1]);
			if (c <= ' ')
			    break;
		    }
		    p++;
		    switch (c) {
			case 'h':  b = 7; break;
			case 's':  b = 6; break;
			case 'p':  b = 5; break;
			case 'a':  b = 4; break;
			case 'r':  b = 3; break;
			case 'w':  b = 2; break;
			case 'e':  b = 1; break;
			case 'd':  b = 0; break;
			default:  b = -1;
		    }
		    if (b < 0)
			putn(
" *** letters after -P must be any of H S P A R W E D.");
		    else {
			protlook |= bit(b);
			if (tilt ^ (b >= 4))
			    protwant |= bit(b);
		    }
		}
		continue;
	    }
	    case 'a':  case 'b':  case 'n':  {
		register short age = -1;
		register char cc;
		while ((cc = p[1]) >= '0' && cc <= '9') {
		    if (age < 0) age = cc - '0';
		    else age = 10 * age + cc - '0';
		    p++;
		}
		if (c == 'a')
		    after = age + 1;
		else if (c == 'b')
		    before = age + 1;
		else {
		    if (cupside = (age >= 10))
			age -= 10;
		    if (age >= 0 && age <= TOPSTYLE)
			steil = age;
		    else
			putfmt(" *** -N must be followed by a number from "
					"0 to %d or 10 to %d.\n",
					TOPSTYLE, TOPSTYLE + 10);
		}
		continue;
	    }
	    case '{':				/* lower('[') == '{' */
		if (*(p++) == '[') {
		    if (*p) oform = (str) p;
		    else oform = null;
		} else {
		    if (*p) xform = (str) p;
		    else xform = null;
		}
		return;
	}
	putfmt(" *** Unknown option -%c.\n", toupper(*p));
    }
}



void DoEnvOpts(void)
{
    ubyte brack;
    ustr p, q, hole, op = abspath;
    short l;
    APTR owp = me->pr_WindowPtr;

    me->pr_WindowPtr = (APTR) -1;
    l = GetVar("DR-OPTS", op, ABSIZE, 0L);
    me->pr_WindowPtr = owp;
    if (l <= 0)
	return;
    do {
	hole = null;
	for (p = op; *p > ' '; p++)
	    if (*p == '[' || *p == '{') {
		brack = *p + 2;			/* [ -> ], { -> } */
		for (q = p + 1; *q && *q != brack; q++)
		    if (*q == '\\' && q[1])
			q++;
		if (*q) {
		    *(hole = q) = 0;
		    break;
		}
	    }
	Opt(op - 1);
	if (hole)
	    op = hole + 1;
	else
	    while (*op > ' ') op++;
	while (*op && *op <= ' ') op++;
    } while (*op);
}



void mane(void)
{
    struct FileHandle *out = gbip(me->pr_COS);
    short a, aa;
    bool help = argc == 2 && (ubyte) *argv[1] == HELP && !argv[1][1];

    if (help) {		/* -cdfhiklorsux | -a# | -b# | -p... | -{...} | -[...] */
	putfmt("\nDr " VERSION " by Paul Kienitz"
		" -- Usage: %s {directories|patterns|-options} ...\n\n"
		"where options are:\n%s", *argv, helpslab);
	abort = 5;
	return;
    }
    if (!cancelenvar)
	DoEnvOpts();
#ifdef DEBUG
 if (cancelenvar) {
  Write(Output(), "Press return: ", 14);
  Read(Input(), &a, 1);
 }
#endif
    color =  !!IsInteractive(me->pr_COS);		/* -1 => 1 */
    wid = CheckWindowWidth();				/* may reset color */
    if (color & cursorhide)
	Write(me->pr_COS, CSI "0 p", 4);		/* cursor off */
    for (aa = argc - 1; aa > 0 && (ubyte) *argv[aa] == HYPH; aa--)
	Opt(argv[aa]);
    morethan1 = argc - hyphc > 2;
    if (aa) {
	for (a = 1; a <= aa && !abort; a++)
	    if ((ubyte) *argv[a] == HYPH)
		Opt(argv[a]);
	    else {
		if ((ubyte) *argv[a] == HELP) *argv[a] = '?';
		Do(argv[a]);
	    }
    } else
	Do("");
    if (needsnl)
	puch('\n');
    if (hair && !ctifle && hair != ERROR_OBJECT_NOT_FOUND
			&& hair != ERROR_OBJECT_WRONG_TYPE && !abort) {
	PFault("\n*** LISTING NOT COMPLETE");
    }
    pflush();
    if (color & cursorhide)
	Write(me->pr_COS, CSI "1 p", 4);	/* cursor on */
}



/* This version of cliparse NO LONGER uses *" to mean a quote inside quotes;
   instead it uses "", because we need * for wildcards now.  It marks -args with
   a special character (HYPH), accepting a -arg in quotes as a filename.  *N and
   *E are not supported, since filenames can't contain newline or escape.  It
   parses strings inside -[ ] or -{ } without stopping for spaces.  It converts
   any unquoted question mark at the start of a word into another special
   character (HELP).
 */

void cliparse(long alen, str aptr)
{
    register short coml;
    bool quoted, litting, lit, hyped = false;
    register char c;
    char litter, unquote;
    str tempargv[200];
    register str ap = aptr;
    register str poik = bip(char, bip(struct CommandLineInterface,
				      me->pr_CLI)->cli_CommandName);

    hyphc = 0;
    coml = *poik;
    arglen = coml + (short) alen + ((short) alen >> 2) + 1;
    if (!(argline = Alloc(arglen))) return;
    strncpy(argline, poik + 1, (size_t) coml);
    argline[coml] = '\0';
    *tempargv = argline;
    argc = 1;
    aptr = ap + alen;		/* now marks the end instead of the start */
    if (alen > 0) {
	tempargv[1] = poik = argline + coml + 1;
	for (;;) {
	    if ((ubyte) *ap <= ' ') {
		hyped = false;
		while (ap < aptr && (ubyte) *ap <= ' ') ap++;
	    }
	    if (ap >= aptr) break;
	    if (*ap == '-') {
		hyphc++;
		*(poik++) = HYPH;
		ap++;
		hyped = true;
	    }
	    quoted = lit = false;
	    if (hyped && lower(*ap) == '{') {
		quoted = true;
		litter = '\\';
		if (*ap == '[') unquote = ']';
		else unquote = '}';
	    } else if (!hyped && *ap == '"') {
		ap++;
		quoted = true;
		litter = unquote = '"';
	    } else if (!hyped && *ap == '?')
		*ap = HELP;
	    while (c = *ap, litting = quoted && !lit && c == litter
					&& (litter != '"' || ap[1] == '"'),
			(ap < aptr && (quoted ? lit || litting || c != unquote
					      : (ubyte) c > ' ')
			 && (!hyped || quoted || lower(c) != '{'))) {
		lit = litting;
		if (!lit || litter == '\\')
		    *(poik++) = c;
		if (hyped && !quoted) {
		    if (c == '!')
			cursorhide = true;
		    else if (c == '@')
			cancelenvar = true;
		    else if (c == '?')
			ctifle = true;
		}
		ap++;
	    }
	    if (quoted && *ap == unquote) ap++;
	    *(poik++) = '\0';
	    tempargv[++argc] = poik;
	    if ((quoted && litter == '\\' && (ubyte) *ap > ' ')
				|| (hyped && lower(c) == '{')) {
		*(poik++) = HYPH;	/* kluge for args around [] or {} */
		hyphc++;
	    }
	}
    }
    tempargv[argc] = null;
    if (!(argv = Alloc((argc + 1) << 2))) {
	argc = 0;
	FreeMem(argline, (long) arglen);
    }
    for (coml = 0; coml <= argc; coml++)
	argv[coml] = tempargv[coml];
}



/* simplified reentrant startup code */

long _main(long alen, str aptr)
{
    adr reta;

    me = ThisProcess();
    reta = me->pr_ReturnAddr;
    if (!me->pr_CLI) {
	/**** maybe reply wbstartup msg here?  naah */
	return 999999L;		/* never gets unloaded by workbench */
    }
    stacklimit = (adr) ((str) reta - *(long *) reta + 4);
    if (((struct Library *) DOSBase)->lib_Version < 37) {
	Write(me->pr_COS, (ustr)
		"This version of Dr requires AmigaDOS 2.04 or newer.\n", 52L);
	me->pr_Result2 = ERROR_INVALID_RESIDENT_LIBRARY;
	return 20L;
    }
    if (OpenSmallIO(CCch)) {
	me->pr_Result2 = ERROR_NO_FREE_STORE;
	return 20L;
    }
    colsort = colorful = true;
    cliparse(alen, aptr);
    if (argc)
	mane();
    else {
	putfmt("Dr:  Not enough memory to start up!\n");
	hair = ERROR_NO_FREE_STORE;
	abort = 20;
    }
    if (argline) {
	FreeMem(argline, (long) arglen);
	FreeMem(argv, (long) (argc + 1) << 2);
    }
    CloseSmallIO();
    if (hair) {
	me->pr_Result2 = hair;
	if (!abort) abort = (hair == ERROR_LINE_TOO_LONG ? 5 : 10);
    }
    if (ctifle)
	if (abort == 1)
	    abort = 0, me->pr_Result2 = 0;
	else if (!abort)
	    abort = 5, me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;
    return (long) abort;
}
