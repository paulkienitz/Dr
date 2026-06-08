/* This is fascl.c, source for fastscan.library -- based on fastex.c routines
which were linked into Dr (versions 1.0 thru 1.3) but are now separate.
This source file and fastscan.library are Copyright © 1992 by Paul Kienitz, and
are freely distributable as long as this notice is included in easily noticed
form.  The library may be distributed without the source or the documentation
file (with copyright notice), but otherwise the source, documentation, and
library should be distributed together. */

/* NEEDED:
Handle hard trackdisk error in DigestTrack; never look at track again? Look for
	other ways to be robust when dealing with corrupt data on disk.
Find out how to correctly handle environment DE_MASK value.
Non-512-byte blocks someday?
Implement RexxQuery, in an alternate version (using #ifdefs) ?
*/

/* ======================================================================= */


#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <libraries/filehandler.h>
#include <libraries/dosextens.h>
#include <intuition/intuition.h>
#include <string.h>
#include <Paul.h>


/******
  THE FOLLOWING MATERIAL IS LIFTED ALMOST VERBATIM from the Aztec C library
  building block file libstart.asm ::::::::
******/

#asm

; Copyright (C) 1986,1987 by Manx Software Systems, Inc.

;	libstart.a68 -- romtag for library

	include	'exec/types.i'
	include	'exec/resident.i'
	include	'exec/nodes.i'
	include	'exec/libraries.i'


FSLVERSION	equ	102		; you may laugh but I have my reasons
FSLPRI		equ	0

	cseg	; romtag must be in first hunk

	public	_fslname
	public	_fslid
	public	_fslInitTab

	entry	no_run
no_run:
	moveq	#100,d0		; don't let them run me
	rts
	public	_fslRomTag
_fslRomTag:
	dc.w	RTC_MATCHWORD
	dc.l	_fslRomTag
	dc.l	endtag
	dc.b	RTF_AUTOINIT
	dc.b	FSLVERSION
	dc.b	NT_LIBRARY
	dc.b	FSLPRI
	dc.l	_fslname
	dc.l	_fslid
	dc.l	_fslInitTab
	dc.w	0		;to get things aligned to 4 byte boundary


;	For libraries:
;		library base in D0
;		segment list in A0
;		execbase in A6

;	Initial startup routine for Aztec C.

;	NOTE: code down to "start" must be placed at beginning of
;		all programs linked with Aztec Linker using small
;		code or small data.


	public	.begin
.begin
	public	_fslInit
_fslInit:
	movem.l	d0-d7/a0-a6,-(sp)	
	movem.l	d0/a0,-(sp)		;save library parameters
	bsr	_geta4			;get A4
	lea	__H1_end,a1
	lea	__H2_org,a0
	cmp.l	a1,a0			;check if BSS and DATA together
	bne	start			;no, don't have to clear
	move.w	#((__H2_end-__H2_org)/4)-1,d1
	bmi	start			;skip if no bss
	move.l	#0,d0
loop
	move.l	d0,(a1)+		;clear out memory
	dbra	d1,loop

start
	move.l	a6,_SysBase		;put where we can get it

	lea	dos_name,a1		;get name of dos library
	jsr	-408(a6)		;open the library any version
	move.l	d0,_DOSBase		;set it up
	bne	3$			;skip if okay

  	move.l  #$38007,d7		;AG_OpenLib | AO_DOSLib
	jsr     -108(a6)		;Alert
	bra	4$
3$
	jsr	__main			;call the startup stuff
4$
	add.w	#8,sp			;pop args
	movem.l	(sp)+,d0-d7/a0-a6
	rts				;and return

dos_name:
	dc.b	'dos.library',0

	public	_geta4
_geta4:
	far	data
	lea	__H1_org+32766,a4
	rts

	public	__main,__H0_org

	dseg

	public	_SysBase,_DOSBase
	public	__H1_org,__H1_end,__H2_org,__H2_end
	bss	_SysBase,4
	bss	_DOSBase,4

	cseg

#endasm


/******
  THE FOLLOWING MATERIAL IS LIFTED ALMOST VERBATIM from the Aztec C library
  building block file libsup.c ::::::::
******/


struct FastScanBase {
    struct Library	fb_Lib;
    unsigned long	fb_SegList;		/* seglist of lib itself */
    short		fb_fibcount;		/* count of actual usages */
};

/* fb_fibcount is unused for the present.  If someone closes the library
without FastExCleanuping some fib, that's THEIR problem. */


struct FastScanBase *FastScanBase;	/* otherwise #pragma no work */

extern void *SysBase;


#define fib struct _fib


void fslInit(void);
long fslOpen(void);
long fslClose(void);
long fslExpunge(void);

long FSRexxQuery(struct Message *rm);
long FastExamine(BPTR lok, fib *fibb);
long FastExNext(BPTR lok, fib *fibb);
void FastExCleanup(fib *fibb);
void *FastExGet80(fib *fibb);


#pragma amicall(FastScanBase, 0x06, fslOpen())
#pragma amicall(FastScanBase, 0x0C, fslClose())
#pragma amicall(FastScanBase, 0x12, fslExpunge())

#pragma amicall(FastScanBase, 0x1E, FSRexxQuery(a0))   /* ARexx passes msg in A0 */
#pragma amicall(FastScanBase, 0x24, FastExamine(d0, a0))
#pragma amicall(FastScanBase, 0x2A, FastExNext(d0, a0))
#pragma amicall(FastScanBase, 0x30, FastExCleanup(a0))
#pragma amicall(FastScanBase, 0x36, FastExGet80(a0))


/* library initialization table, used for AUTOINIT libraries */
struct InitTable {
    unsigned long  it_DataSize;		 /* library data space size        */
    void	  **it_FuncTable;	 /* table of entry points          */
    void 	  *it_DataInit;		 /* table of data initializers     */
    void	  (*it_InitFunc)(void);	 /* initialization function to run */
};

void *libfunctab[] = {
    fslOpen,			/* standard open     */
    fslClose,			/* standard close    */
    fslExpunge,			/* standard expunge  */
    0,				/* reserved */
    FSRexxQuery,		/* a null stub for now */
    FastExamine,
    FastExNext,
    FastExCleanup,
    FastExGet80,
    (void *) -1			/* end of function vector table */
};

struct InitTable fslInitTab =  {
    sizeof (struct FastScanBase),
    libfunctab,
    0,
    fslInit
};

#define FSLREVISION 6

#ifdef DEBUG
char fslname[] = "dfastscan.library";
#else
char fslname[] = "fastscan.library";
#endif
char fslid[] = "fastscan 102.6 (" __DATE__ ")\n\r";
char credit[] = "Copyright © 1992 by Paul Kienitz\n";

extern struct Resident	fslRomTag;


void *_main(struct FastScanBase *Fsb, unsigned long seglist)
{
    register struct Library *fsl = &Fsb->fb_Lib;

    /* ----- init. library structure ----- */
    fsl->lib_Node.ln_Type = NT_LIBRARY;
    fsl->lib_Node.ln_Name = (char *) fslname;	
    fsl->lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    fsl->lib_Version = fslRomTag.rt_Version;
    fsl->lib_Revision = FSLREVISION;
    fsl->lib_IdString = (APTR) fslid;

    Fsb->fb_SegList = seglist;
    Fsb->fb_fibcount = 0;
    FastScanBase = Fsb;			/* wot the 'ell, init the global */
    return Fsb;
}


long fslOpen(void)
{
    struct FastScanBase *FastScanBase;		/* register a6 */
    FastScanBase->fb_Lib.lib_OpenCnt++;
    /* prevent delayed expunges (standard procedure) */
    FastScanBase->fb_Lib.lib_Flags &= ~LIBF_DELEXP;
    return ((long) FastScanBase);
}


long fslClose(void)
{
    struct FastScanBase *FastScanBase;		/* register a6 */
    long retval = 0;

    if (--FastScanBase->fb_Lib.lib_OpenCnt == 0) {
	if  (FastScanBase->fb_Lib.lib_Flags & LIBF_DELEXP) {
	    retval = fslExpunge();		 /* return segment list */
	}
    }
    return (retval);
}


long fslExpunge(void)
{
    struct FastScanBase *FastScanBase;		/* register a6 */
    unsigned long seglist = 0;
    long libsize;
    extern struct DosLibrary *DOSBase;

    if (FastScanBase->fb_Lib.lib_OpenCnt == 0) {
	seglist = FastScanBase->fb_SegList;
	Remove(&FastScanBase->fb_Lib.lib_Node);
	libsize = FastScanBase->fb_Lib.lib_NegSize+FastScanBase->fb_Lib.lib_PosSize;
	FreeMem((char *)FastScanBase-FastScanBase->fb_Lib.lib_NegSize, libsize);
	CloseLibrary((struct Library *) DOSBase);
    } else
	FastScanBase->fb_Lib.lib_Flags |= LIBF_DELEXP;
    return ((long) seglist);
}

/**** end of stolen material **********/



/***************************
   Here we get into the FastExamine / FastExNext routines that were once
   part of Dr:
***************************/




/* =============================== Here are some types and constants: */

/* in case of out-of-date include files like I had: */
#ifndef DOSTRUE
#define DOSTRUE -1L
#define DOSFALSE 0L
#endif

#ifndef DE_DOSTYPE
#define DE_MAXTRANSFER 13
#define DE_MASK        14
#define DE_POOTPRI     15
#define DE_DOSTYPE     16
#endif

typedef struct IOExtTD REEK;
typedef struct IORequest *IORP;


#define tablesize ((TD_SECTOR >> 2) - 56)
    /* GOTTA HANDLE NON-512-BYTE BLOCKS SOMEDAY? ****/

typedef struct {		/* Semi-generic DOS disk block */
    long Type;			/* data = 8, extension = 16, other = 2 */
    long Owner;			/* header pointer for data & ext. blocks */
    long Blocks;		/* in file header, total data blocks used */
    long Size;			/* data blocks in this header's table */
    long First;
    long Checksum;
    long Table[tablesize];	/* the hashtable */
    long Nothing[2];
    long Protection;
    long Length;		/* file size in bytes */
    char Snide[92];
    struct DateStamp Date;
    char Name[36];		/* length in Name[0] */
    long FileLink;		/* disk address we are (hard) linked to */
    long HackLink;		/* reverse list pointer for (hard) linkees */
    long Vacuity[5];
    long Next;			/* next dir entry with same hash value */
    long Parent;		/* owning directory */
    long Stench;		/* next extension block */
    long Type2;		/* file header/ext = -3, directory = 2, root = 1 */
} dosblock;
/* I COULD CARVE A BETTER FILE SYSTEM STRUCTURE OUT OF A BANANA. */

struct _fiblet;
typedef struct _fiblet fiblet;

struct _fiblet {
    fiblet *next;			/* list link (in tree, next=left) */
    fiblet *ready;			/* fiblets ready to return (=right) */
    long key, nexthash, protect, length, blocks;
    struct DateStamp cheap_date;
    str snide;				/* null, or points to 80 bytes */
    long *wanted;			/* if dir, points to table of entries */
    char name[31];			/*    with count in first lword */
    char type;				/* signed int: same as fib_EntryType */
};					/* only 80 bytes, not 260 */


/* This is the "global variables" type stuff: */

typedef struct {
    struct MsgPort ioreply;	/* where IO requests come back */
    REEK *quest;		/* disk IO request (we only need one) */
#define ques quest->iotd_Req
    dosblock *whole_track;	/* track buffer */
    long memtype;		/* track buffer's allocmem flags */
    long changes, unit;		/* floppy disk change count, unit number */
    long flags;			/* device driver flags from fssm */
    ushort readcommand;		/* either ETD_READ or CMD_READ */
    ushort sex, sexize;		/* blocks per track, bytes per block */
    long tracklen;		/* sex * sexize */
    long memmask;		/* forbidden memory for track buffer?? */
    fiblet *maybe;		/* tree of possibly useful files/dirs */
    fiblet *parents;		/* dirs with tables in use, for cleanup (list) */
    fiblet *sausages;		/* tree of unresolved hard and soft links */
    long first_sector;		/* first sector on current track */
    long mid_sector;		/* middle of most recently read track */
    long tish_offset;		/* needed to convert block #s to track #s */
    struct Process *me;		/* our own process node */
    struct DeviceList *vol;	/* volume node on dos device list */
    struct MsgPort *lask;	/* original dos handler of lock */
    fiblet *lastwho;		/* for seeing when Checkbies is necessary */
    fiblet *outwho;		/* parentdir of scan that sent quest out */
    long outkey;		/* block that quest was sent out after */
    bool devopen;		/* the disk's xxxx.device is open */
    bool ffs;			/* it's fast file system */
    bool dos2;			/* we are under AmigaDOS 2.0 or later */
    bool depth;			/* we are prepared for recursive descent */
    bool pending;		/* an IORequest is out there being worked on */
    bool trackdisk;		/* this is a regular trackdisk.device floppy */
    short blize;		/* the amount of data in a data block */
    short sigbit;		/* shared by all of our msgports */
    char debna[64];		/* device driver name string */
} state;

#define SS register state *s


#define pack struct _pack

pack {
    pack *chain;
    struct FileInfoBlock fibuf;
    struct StandardPacket sp;
    struct MsgPort repp;
    bool isout;
    BPTR lock;
};				/* for FALLASYNCH | FALLASDEEP */



/* struct */ fib {		/* an extended FileInfoBlock */
    struct FileInfoBlock f;
    fiblet *pard;		/* ready & wanted currently being used */
				/* pard is type (pack *) when FALLASYNCH */
    state *ss;			/* state o' th' scan */
};

/* #define MAGICVALUE 1253761265 */


/* special values for the ss field (others are assumed to be pointers): */

#define NEWSHALLOW  0L
#define NEWDEEP    -1L
#define CLEANED    -2L
#define FALLTHEWAY -3L
#define FALLASDEEP -4L
#define FALLASYNCH -5L


import struct DosLibrary *DOSBase;

adr IntuitionBase;



/* ================================= And now, functions */

/* First the lower level stuff... */



fiblet *Pop(fiblet **lis)
{
    register fiblet *r = *lis;
    if (r) *lis = r->next;
    return r;
}



void Push(fiblet **lis, fiblet *v)
{
    v->next = *lis;
    *lis = v;
}



fiblet *ExtractLowest(register fiblet **t)
{
    fiblet *r;
    if (!*t) return null;
    while ((*t)->next) t = & (*t)->next;
    r = *t;
    *t = r->ready;
    return r;
}



adr GetFiblet(SS)
{
    register fiblet *r;
    if (!NEW(r))
	r = ExtractLowest(&s->maybe);			/* DESPERATION! */
    if (!r) s->me->pr_Result2 = ERROR_NO_FREE_STORE;
    return r;
}



void Freeblet(fiblet *f)
{
    if (f->snide) FreeMem(f->snide, (long) sizeof(fiblet));
    if (f->wanted)
	FreeMem(f->wanted, (*(f->wanted) + 1L) << 2);
    FREE(f);
}



#ifndef AZTEC_C
#define OLD_HIGH_LEVEL_VERSION
#endif

#ifdef OLD_HIGH_LEVEL_VERSION

/* see, I did some profiling and concluded that these tree functions are (not
surprifingly) the ones that most want optimizing.  second most wanting
functions are Chew and ChewSomeMore.  ExtractLowest doesn't need it.  So the
functions within this #if have been rewritten in assembly.  */

/* the idea here is that we wanna hash up the key that the tree is sorted
by cuz the tree tends ta get filled with consecutive values so it gets lots
of long stringy diagonals without branches all in one direction. */

ulong Hash(register ulong k)
{
    ulong d = ((k & 63) << 26) | (k >> 6);
    return d ^ 0x55555555;
}		/* what I really want is reversed significance of bits */



fiblet *Extract(register fiblet **tree, long kee)
{
    register fiblet *t;
    long tk;
    kee = Hash(kee);
    while (*tree && (tk = Hash((*tree)->key)) != kee)
	if (tk > kee)
	    tree = & (*tree)->next;
	else tree = & (*tree)->ready;
    if (*tree) {      /* tree points to pointer that points to right one */
	t = *tree;
	if (!t->next)
	    *tree = t->ready;
	else if (!t->ready)
	    *tree = t->next;
	else {
	    fiblet *tt = ExtractLowest(&t->ready);
	    *tree = tt;
	    tt->next = t->next;
	    tt->ready = t->ready;
	}
	t->next = t->ready = null;
	return t;
    } else return null;
}

#else


/* and now here are the optimized versions: */
#asm
	public	_Hash

_Hash:	move.l	4(sp),d0		; the arg
	ror.l	#6,d0			; RoR, gentlemen...   RoReth
	eor.l	#$55555555,d0		; alucard
	rts				; pigdog
#endasm

import ulong Hash(ulong k);



#asm
	public	_Extract

_Extract:
	movem.l	d5-d7/a3-a5,-(sp)	; play it safe
	move.l	28(sp),a5		; tree
	move.l	32(sp),d7		; kee
	ror.l	#6,d7			; INLINE version of
	eor.l	#$55555555,d7		;   kee = Hash(kee)

while:	tst.l	(a5)
	beq	endwh
	move.l	(a5),a4
	move.l	8(a4),d6		; (*tree)->key
	ror.l	#6,d6			; Hash it inline
	eor.l	#$55555555,d6
	cmp.l	d6,d7
	beq	endwh
	move.l	(a5),a5			; tree = & (*tree)->next
	blt	while			; if Hash((*tree)->key) < Hash(kee)
	addq	#4,a5			;   tree = & (*tree)->ready
	bra	while

endwh:	move.l	(a5),a4			; the node we'll return
	move.l	a4,d0			; pseudo tst.l
	beq	ret			; if !*tree return (null)
	tst.l	(a4)			; if !(*tree)->next
	bne	rite
	move.l	4(a4),(a5)		;   *tree = (*tree)->ready
	bra	finn

rite:	tst.l	4(a4)			; else if !(*tree)->ready
	bne	uh_oh
	move.l	(a4),(a5)		;   *tree = (*tree)->next
	bra	finn

uh_oh:	move.l	a4,d5
	addq	#4,d5			; pseudo lea 4(a4),d5
	move.l	d5,-(sp)		; tt = ExtractLowest(&returnee->ready)
	bsr	_ExtractLowest
	addq	#4,sp
	move.l	d0,a3
	move.l	a3,(a5)			; *tree = tt
	move.l	(a4),(a3)		; tt->next = returnee->next
	move.l	4(a4),4(a3)		; tt->ready = returnee->ready

finn:	clr.l	(a4)			; returnee->next = null
	clr.l	4(a4)			; returnee->ready = null
	move.l	a4,d0
ret:	movem.l	(sp)+,d5-d7/a3-a5
	rts
#endasm

import fiblet *Extract(fiblet **tree, long kee);

/* Well, damn.  The speed gain from that was not measureable.  That routine
had in the past cost significant time when it was being called too often by
Checkbies, but not any more.  Oh well, as long as it's done, there's no
reason to take it out... */

#endif



void Insertt(register fiblet **tree, fiblet *f)
{
    long kee = Hash(f->key), tk;
    while (*tree) {
	tk = Hash((*tree)->key);
	if (tk > kee)
	    tree = & (*tree)->next;
	else
	    tree = & (*tree)->ready;
    }
    *tree = f;
}



void FlushList(fiblet **lis)
{
    register fiblet *foo;
    while (foo = Pop(lis))
	Freeblet(foo);
}



void FlushTree(register fiblet *t)
{
    if (t) {
	FlushTree(t->next);
	FlushTree(t->ready);
	Freeblet(t);
    }
}



void WeWant(long k, register long *w)
{
    register short t;
    for (t = 1; t <= *w; t++)
	if (!w[t]) {
	    w[t] = k;
	    return;
	}
}



bool YankIfPresent(long k, register long *w)
{
    register short t;
    if (!k) return false;		/* occasionally needed! */
    for (t = 1; t <= *w; t++)
	if (w[t] == k) {
	    w[t] = 0;
	    return true;
	}
    return false;
}



long Nearest(long k, long *w)
{
    register short t;
    register long c, d = maxlong, e = 0;
    for (t = 1; t <= *w; t++)
	if (c = w[t]) {
	    c -= k;
	    if (c < 0) c = -c;
	    if (c <= d) {
		d = c;
		e = w[t];
	    }
	}
    return e;
}



void btoc(str c, ubyte *b, short lim)		/* b == c is okay */
{
    register ushort l = *b;
    if (l > lim) l = lim;
    strncpy(c, (str) b + 1, (size_t) l);
    c[l] = '\0';
}



void Chew(register dosblock *b, register fiblet *z, short l, fiblet *fake)
/* for now assume always 512 bytes */
{
    bool hardon = b->Type2 == 4 || b->Type2 == -4;

    if (fake) {
	*z = *fake;
	z->key = fake->length;
    } else {
	btoc(z->name, (ubyte *) b->Name, 30);
	z->cheap_date = b->Date;
	z->nexthash = b->Next;
	z->type = b->Type2;
	z->key = (hardon ? b->FileLink : b->Owner);
    }
    if (b->Type2 == 1) {		/* root directory */
	z->protect = z->length = 0;
	z->type = 2;
    } else {
	z->protect = b->Protection;
	z->length = (hardon ? b->Owner : b->Length);
    }
    z->blocks = (hardon ? -1 : (z->length + l - 1) / l);   /* ESTIMATE! */
/* we do NOT transfer filenotes to hard links as of dos 2.04: */
/* if this ever changes, make sure to keep Chew and GotASec consistent! */
    if (!fake && *b->Snide && (z->snide = Alloc(sizeof(fiblet))))
	btoc(z->snide, (ubyte *) b->Snide, 79);	   /* ^^ GetFiblet?  nah. */
    else
	z->snide = null;	/* who cares if we lose a filenote? */
    z->next = z->ready = null;
    z->wanted = null;
}

/* when we are passed a hard link (Type2 = -4 or 4), we treat it weirdly ...
we put its own key in the length field, and in the key field we put the key
of the block it is linked to.  This is because link fiblets get stored in a
tree (s->sausages) and we want to look it up by the key of what it is linked
to, using Extract().  We use -1 in the blocks field to distinguish unresolved
hard links from resolved ones.  Soft links we do not resolve -- the Lock
function will resolve them for anyone doing recursive descent. */



bool DirChew(dosblock *b, fiblet *z, short l)
{
    register short x, s = 1;
    Chew(b, z, l, null);
    if (z->type <= 0) return false;
    for (x = 0; x < tablesize; x++)	/* assumes 512 byte blocks ****/
	if (b->Table[x]) s++;
    if (z->wanted = Alloc(s << 2)) {
	register long t;
	*(z->wanted) = s - 1;
	s = 1;
	for (x = 0; x < tablesize; x++)
	    if (t = b->Table[x]) z->wanted[s++] = t;
	return false;
    }
    return true;
}



void ChewSomeMore(register fiblet *f, register fib *fibb)
{
    fibb->f.fib_DiskKey = f->key;
    fibb->f.fib_DirEntryType = fibb->f.fib_EntryType = f->type;
    fibb->f.fib_Protection = f->protect;
    fibb->f.fib_Size = f->length;
    fibb->f.fib_NumBlocks = f->blocks;
    fibb->f.fib_Date = f->cheap_date;
    strcpy(fibb->f.fib_FileName, f->name);
    if (f->snide) strcpy(fibb->f.fib_Comment, f->snide);
    else fibb->f.fib_Comment[0] = 0;
}



#define xt struct IntuiText

xt retry = { 0, 1, JAM2, 6, 3, null, (ustr) "Retry", null };
xt cancel = { 0, 1, JAM2, 6, 3, null, (ustr) "Cancel", null };



void mixtline(register xt *t, short y, str s, xt *n)
{
    t->FrontPen = 0;
    t->BackPen = 1;
    t->DrawMode = JAM2;
    t->LeftEdge = 15;
    t->TopEdge = y;
    t->ITextFont = null;
    t->IText = (ustr) s;
    t->NextText = n;
}



void mixtmess(register xt *t, str s1, str v, str s3, SS)
{
    mixtline(t + 2, 25, s3, null);
    mixtline(t + 1, 15, v, t + 2);
    mixtline(t, 5, s1, t + 1);
}



/* ================= Higher level routines */


struct FileSysStartupMsg *FindStartup(struct MsgPort *lask, SS)
{
    struct FileSysStartupMsg *fart = null;
    struct DosList *debb;
    if (s->dos2) {
	debb = LockDosList(LDF_DEVICES | LDF_READ);
	for (debb = NextDosEntry(debb, LDF_DEVICES);
		    debb; debb = NextDosEntry(debb, LDF_DEVICES))
	    if (debb->dol_Task == lask)
		fart = gbip(debb->dol_misc.dol_handler.dol_Startup);
    } else {
	BPTR dog = ((struct RootNode *) DOSBase->dl_Root)->rn_Info;
	struct DosList *devlist = gbip(bip(struct DosInfo, dog)->di_DevInfo);
	Forbid();
	for (debb = devlist; debb; debb = gbip(debb->dol_Next))
	    if (debb->dol_Type == DLT_DEVICE && debb->dol_Task == lask)
		fart = gbip(debb->dol_misc.dol_handler.dol_Startup);
    }
    if ((ulong) fart < 1024 || !TypeOfMem(fart)
				|| !TypeOfMem(gbip(fart->fssm_Environ)))
	return null;
    return fart;
}



void ReleaseStartup(SS)
{
    if (s->dos2)
	UnLockDosList(LDF_DEVICES | LDF_READ);
    else
	Permit();
}



bool Quest(APTR wptr, xt *tx, ulong flig)	/* for dos 1.3 and older */
{
    return ~(long) wptr && IntuitionBase
	   && AutoRequest((adr) wptr, tx, &retry, &cancel, flig, 0L, 320L, 72L);
}



/* This is called by GetTrack when WaitIO returns an error.  It
returns true if the operation should be retried. */

bool ManageError(SS)
{
    short air = s->ques.io_Error;
    char volume[61];
    xt lines[3];
    long hair = 0;
    APTR wptr = s->me->pr_WindowPtr;
    str vone = gbip((BSTR) s->vol->dl_Name);	/* who put BSTR * in there?! */
    ulong iflags;
    static struct EasyStruct zz1 =
		{ sizeof(struct EasyStruct), 0, null,
		  "Please replace volume\n%s\nin any drive", "Retry|Cancel" },
		zz2 = { sizeof(struct EasyStruct), 0, null,
		  "%s\nhas a read error\non disk block %ld", "Retry|Cancel" };

/**** WHAT ABOUT TDERR_DriveInUse??  Can it happen? */
    
    strncpy(volume, vone + 1, (size_t) *vone);
    volume[*vone] = 0;
    IntuitionBase = OpenLibrary("intuition.library", 0L);
    if (air == TDERR_NoMem)
	hair = ERROR_NO_FREE_STORE;
    if (s->vol->dl_Task != s->lask)		/* might help a little bit */
	air = TDERR_DiskChanged;
    else if (air == TDERR_DiskChanged || air == 11 /* UNDOCUMENTED ****/ ) {
	hair = ERROR_DEVICE_NOT_MOUNTED;
	mixtmess(lines, "Please replace volume", volume, "in any drive", s);
	do {
	    if (s->vol->dl_Task && s->vol->dl_Task != s->lask) {
		struct FileSysStartupMsg *fart = FindStartup(s->vol->dl_Task, s);
		ubyte *den = gbip(fart->fssm_Device);
		if (fart && !strncmp(s->debna, (str) den + 1, (size_t) *den)) {
		    if (s->devopen) CloseDevice((IORP) s->quest);
		    s->unit = fart->fssm_Unit;
		    s->flags = fart->fssm_Flags;
		    if (!OpenDevice((ustr) s->debna, s->unit,
					(IORP) s->quest, s->flags))
			s->lask = s->vol->dl_Task;
		    else s->devopen = false;
		}
		ReleaseStartup(s);
	    }
	    if (s->lask == s->vol->dl_Task) {
		hair = 0;
		s->ques.io_Command = TD_CHANGENUM;
		DoIO((IORP) s->quest);
		s->changes = s->ques.io_Actual;
		break;
	    }
	} while (IntuitionBase && (s->dos2
/****		   ? !ErrorReport((long) ERROR_DEVICE_NOT_MOUNTED,
				 (long) REPORT_VOLUME, (ulong) s->vol, null) */
/* the above ErrorReport SHOULD work, but it fails to show the volume name (it
shows blanks), and it does not respond to disk insertion.  So:  */
		   ? (iflags = DISKINSERTED,
		      EasyRequest(wptr, &zz1, &iflags, volume))
		   : Quest(wptr, lines, (ulong) DISKINSERTED)));
    } else if (s->readcommand == ETD_READ && air <= TDERR_NotSpecified)
	s->readcommand = CMD_READ;	/* ETD_READ unsupported (lame driver) */
    else if (s->sex > 1 && !s->trackdisk) {
	s->sex = 1;				/* retry single block only */
	s->tracklen = s->sexize;
	return true;
    } else {
	if (~(long) wptr) s->me->pr_WindowPtr = null;	      /* authenticity */
	if (s->dos2) {
/* ErrorReport does not work here, because instead of reporting "read error on
disk block N" like the official system requester does, it only says "read/write
error" like the old dos 1.3 requester.  So:  */
	    iflags = 0;
	    if (!IntuitionBase || !EasyRequest(null, &zz2, &iflags,
						volume, s->outkey))
		hair = ERROR_NOT_A_DOS_DISK;	/* best we can do */
	} else {
	    mixtmess(lines, "Volume", volume, "has a read/write error", s);
	    if (!Quest(s->me->pr_WindowPtr, lines, 0L))
		hair = ERROR_NOT_A_DOS_DISK;	/* best we can do */
	}
	s->me->pr_WindowPtr = wptr;
    }
    if (IntuitionBase) CloseLibrary(IntuitionBase);
    if (hair) {
	s->me->pr_Result2 = hair;
	return false;
    }
    return true;
}



void FindDrive(SS)
{
    struct FileSysStartupMsg *fart;
    long *feh;
    ulong mat = 0xffffffffL;
    short divr;
    ubyte *deb;

    s->dos2 = DOSBase->dl_lib.lib_Version >= 36;
    fart = FindStartup(s->lask, s);
    if (fart && (feh = gbip(fart->fssm_Environ))
			&& (deb = gbip(fart->fssm_Device), *deb <= 59)) {
	strncpy(s->debna, (str) deb + 1, (size_t) *deb);
	s->debna[*deb] = 0;
	s->trackdisk = !strcmp(s->debna, "trackdisk.device");
	s->unit = fart->fssm_Unit;
	s->flags = fart->fssm_Flags;
	s->sex = feh[DE_BLKSPERTRACK];
	s->sexize = feh[DE_SIZEBLOCK] << 2;
	s->tish_offset = feh[DE_LOWCYL] * feh[DE_NUMHEADS] * s->sex;
	if (feh[DE_TABLESIZE] >= DE_MAXTRANSFER) {
	    mat = (ulong) feh[DE_MAXTRANSFER] / s->sexize;
	    if (mat && mat < s->sex) {
		if (s->sex % mat == 0)
		    s->sex = mat;	/* pretend tracks are smaller */
		/* else simply ignore maxtransfer */
	    }
	}
	if (s->sex != mat)
	    for (divr = s->sex / 11; divr >= 2; divr--)
		if (!(s->sex % divr)) {
		    s->sex /= divr;
		    break;		/* take smaller bites; faster sometimes */
		}
	if (feh[DE_TABLESIZE] >= DE_MASK)
	    s->memmask = feh[DE_MASK];
	else s->memmask = 0xffffffffL;
	if (feh[DE_TABLESIZE] >= DE_DOSTYPE) {
	    mat = feh[DE_DOSTYPE] & 2;		/* handle international FSs */
	    if (!(s->ffs = (mat == 0x444f5301L)))
		if (mat != 0x444f5300L)
		    s->unit = -1;
	} else
	    s->ffs = false;
	if (!s->sex || s->sexize != 512 || (s->ffs && !s->depth))
	    s->unit = -1;
	/* ^v^v can't do non-512-byte blocks yet */
	s->blize = (s->ffs ? 512 : 488);
	s->tracklen = s->sex * s->sexize;
	s->memtype = feh[DE_TABLESIZE] >= DE_MEMBUFTYPE ?
			feh[DE_MEMBUFTYPE] : MEMF_PUBLIC | MEMF_CHIP;
    }
    ReleaseStartup(s);
}



void InitPort(struct MsgPort *p, short sigbit)
{
    void NewList(struct List *l);
    /* we assume (require) that *p is all zeroes before sent here */
    p->mp_Node.ln_Type = NT_MSGPORT;
    p->mp_Flags = PA_SIGNAL;
    p->mp_SigBit = sigbit;
    p->mp_SigTask = (adr) ThisProcess();
    NewList(&p->mp_MsgList);
    p->mp_MsgList.lh_Type = NT_MSGPORT;		/* what the heck */
}



short OpenIt(SS)
{
    register short air;
    s->ques.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    s->ques.io_Message.mn_Node.ln_Pri = 5;
    s->ques.io_Message.mn_ReplyPort = &s->ioreply;
    InitPort(&s->ioreply, s->sigbit);		
    if (OpenDevice((ustr) s->debna, s->unit, (IORP) s->quest, s->flags))
	return s->ques.io_Error;		/* no IoErr */
    s->devopen = true;
    s->ques.io_Command = TD_CHANGENUM;
    DoIO((IORP) s->quest);
    s->changes = s->ques.io_Actual;
    air = s->ques.io_Error;
    s->readcommand = ETD_READ;		/* until we find it doesn't work */
    return air;				/* no IoErr */
}



void AskTrack(long sect, SS, fiblet *who)
{
    s->first_sector = (sect / s->sex) * s->sex;
    s->mid_sector = s->first_sector + (sect % s->sex) >> 1;
    s->ques.io_Length = s->tracklen;
    s->ques.io_Command = s->readcommand;
    s->ques.io_Data = (adr) s->whole_track;
    s->ques.io_Offset = s->sexize * (s->first_sector + s->tish_offset);
    s->quest->iotd_Count = s->changes;
    s->outkey = sect;
    s->outwho = who;
    if (!s->devopen) return;
    s->pending = true;
    SendIO((IORP) s->quest);
}



void MaybeAskTrack(long sect, SS, fiblet *who)
{
    register fiblet *w;
    if (s->depth)
	for (w = who->ready; w; w = w->next)
	    if (w->type > 0)		/* if any directory ready while depth */
		return;
    AskTrack(sect, s, who);
}



short WaitTrack(SS)
{
    long secks = s->sex;
    if (!s->devopen)
	return TDERR_DiskChanged;
    while (WaitIO((IORP) s->quest) && ManageError(s)) {
	s->ques.io_Command = s->readcommand;
	s->quest->iotd_Count = s->changes;
	SendIO((IORP) s->quest);
    }
    s->sex = secks;
    s->tracklen = secks * s->sexize;
    s->pending = false;
    return s->ques.io_Error;
}



void GotASec(fiblet *foo, SS, fiblet *who)
{
    fiblet *k;
    if (foo->blocks == -1) {
	if (k = Extract(&s->maybe, foo->key)) {	    /* the object linked to */
	    strcpy(k->name, foo->name);		    /* merge link and linkee */
	    k->type = foo->type;
	    k->key = foo->length;
	    k->cheap_date = foo->cheap_date;
	    k->nexthash = foo->nexthash;			/* OOOPS! */
	    if (k->snide) FreeMem(k->snide, sizeof(fiblet));
	    k->snide = null;			/* never show filenote */
	    Freeblet(foo);
	    foo = k;
	} else {
	    Insertt(&s->sausages, foo);		/* unresolved link */
	    WeWant(foo->key, who->wanted);	/* we will look for linkee */
	    return;
	}
    }
    Push(&who->ready, foo);
    if (foo->nexthash) {	    /* nexthash is never already in wanted */
	k = Extract(&s->maybe, foo->nexthash);
	if (k) GotASec(k, s, who);
	else WeWant(foo->nexthash, who->wanted);
    }
}



bool ChewTrack(SS)
{
    short x;
    register dosblock *jerk;
    fiblet *foo, *bar, *sauce;

    for (x = 0; x < s->sex; x++) {
	sauce = null;
	jerk = s->whole_track + x;
	if (jerk->Type == 2 && (jerk->HackLink || (jerk->Type2 != 1 /* root */
				 && (s->depth || jerk->Parent == s->outwho->key)
				 && s->first_sector + x != s->outwho->key))) {
	    sauce = Extract(&s->sausages, jerk->Owner);
	    if (foo = GetFiblet(s)) {
		Chew(jerk, foo, s->blize, sauce);
		if (YankIfPresent(jerk->Owner, s->outwho->wanted)) {
		    GotASec(foo, s, s->outwho);
		    if (jerk->HackLink && (bar = GetFiblet(s))) {
			Chew(jerk, bar, s->blize, sauce);	 /* dublicate */
			Insertt(&s->maybe, bar);	 /* for links to find */
		    }				   /* okay if bar alloc fails */
		} else
		    Insertt(&s->maybe, foo);
		if (sauce) Freeblet(sauce);
		if (s->depth && (foo->type == 2 ||
				 (foo->type == 4 && foo->blocks != -1))) {
		    if (!(bar = GetFiblet(s)))
			return true;
		    if (DirChew(jerk, bar, s->blize)) {
			s->me->pr_Result2 = ERROR_NO_FREE_STORE;
			return true;
		    }
		    Push(&s->parents, bar);
		}
	    } else {
		if (sauce) Insertt(&s->sausages, sauce);	/* un-pop */
		return true;
	    }
	}
    }
    return false;
}



bool DigestTrack(SS)
{
    if (!s->pending) return false;
    if (WaitTrack(s) || ChewTrack(s)) {

/* OUGHT TO DO THIS:  if ERROR_NOT_A_DOS_DISK and driver is trackdisk,   */
/* then strip all blocks from that whole track out of s->outwho->wanted. */

	s->outwho->length = s->me->pr_Result2;
	return true;
    }
    if (!s->outwho->ready)	/* prevent retry of file if it disappears */
	YankIfPresent(s->outkey, s->outwho->wanted);
    return false;
}



struct DeviceList *ValVol(struct FileLock *lick, struct Process *mee)
{
    register struct DeviceList *vawl = gbip(lick->fl_Volume);
    if (vawl->dl_Type != DLT_VOLUME) {			/* not bulletproof */
	mee->pr_Result2 = ERROR_INVALID_LOCK;
	return null;
    }				/* someday test for key in valid range? ****/
    return vawl;
}



void Checkbies(register fiblet *who, SS)
{
    fiblet *foo;
    short x;
    for (x = 1; x <= *(who->wanted); x++)
	if (who->wanted[x] && (foo = Extract(&s->maybe, who->wanted[x]))) {
	    who->wanted[x] = 0;
	    GotASec(foo, s, who);
	}
}



void SendExNext(register pack *p)
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



void WaitExNext(pack *p)
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



bool TrackExamine(BPTR lok, fib *fibb, struct DeviceList *vawl,
				bool *deepe, bool firsty)
{
    register state *s = fibb->ss;
    struct FileLock *lick = gbip(lok);
    long kee = lick->fl_Key;
    fiblet *who = null;
    bool nochew = false;

    if (firsty) {
	fibb->pard = null;
	if (!(fibb->ss = NEWZ(s)))
	    return false;
	if (!~(s->sigbit = AllocSignal(-1L)))
	    goto fallback;
	s->me = ThisProcess();
	s->depth = *deepe;
	s->lask = lick->fl_Task;
	s->unit = -1;
	s->vol = vawl;
	s->me->pr_Result2 = 0;
	s->mid_sector = 0;		/* one should start from the bottom */
	if (NEWPZ(s->quest))
	    FindDrive(s);
	if (s->unit < 0 || !(who = GetFiblet(s))
			|| (who->snide = (adr) who->wanted = null, OpenIt(s))
			|| AvailMem(s->memtype) < s->tracklen + 50000
			|| !(s->whole_track = AllocMem(s->tracklen, s->memtype))
			|| ((long) s->whole_track & ~s->memmask)   /* ???? ****/
			|| (AskTrack(kee, s, who), WaitTrack(s)))
	    goto fallback;
    } else {
	if (DigestTrack(s)) goto fallback;	/* is the block we're examining */
	*deepe = s->depth;			/*    in the parents list? */
	for (who = s->parents; who && who->key != kee; who = who->next) ;
	if (who)
	    nochew = true;		/* yes */
	else if (!(who = GetFiblet(s)) || (AskTrack(kee, s, who), WaitTrack(s)))
	    goto fallback;
    }
    if (!nochew) {
	if (DirChew(s->whole_track + kee % s->sex, who, s->blize))
	    goto fallback;
	if (!who->key) who->key = kee;		/* special case: root dir */
	Push(&s->parents, who);
    }
    ChewSomeMore(who, fibb);
    fibb->pard = who;
    s->me->pr_Result2 = 0;
    if (who->type > 0) {
	if (nochew)
	    Checkbies(who, s);
	else if (ChewTrack(s)) {
	    who = null;				/* prevent freeing twice */
	    goto fallback;
	}
	if (kee = Nearest(s->mid_sector, who->wanted))
	    MaybeAskTrack(kee, s, who);
    } else if (!*deepe)
	FastExCleanup(fibb);			/* not a directory */
    s->lastwho = who;
    return true;
fallback:
    if (!*deepe || firsty)
	FastExCleanup(fibb);
    if (who) Freeblet(who);
    return false;
}



/* ================================ The five visible functions */

adr FastExGet80(fib *f)
{
    register state *s = f->ss;
    if ((long) s >= NEWSHALLOW || (long) s <= FALLASYNCH)
	return GetFiblet(s);
    else return Alloc(80);
}



void FastExCleanup(fib *f)
{
    register state *s = f->ss;
    register fiblet *fent, *temp;

    f->ss = (adr) CLEANED;
    if ((long) s >= FALLTHEWAY && (long) s <= NEWSHALLOW)
	return;
    if ((long) s == FALLASYNCH || (long) s == FALLASDEEP) {
	pack *p = (adr) f->pard;
	short sig = p->repp.mp_SigBit;
	while (p) {
	    f->pard = (adr) p->chain;
	    WaitExNext(p);			/* you can't AbortIO a packet */
	    FREE(p);
	    p = (adr) f->pard;
	}
	FreeSignal((long) sig);
	return;
    }
    if (s->devopen) {
	if (s->pending) {
	    AbortIO((IORP) s->quest);
	    WaitIO((IORP) s->quest);
	}
	s->ques.io_Length = 0;		/* motor off */
	s->ques.io_Command = TD_MOTOR;
	DoIO((IORP) s->quest); 		/* io_Error?  who cares. */
	CloseDevice((IORP) s->quest);
    }
    if (~s->sigbit)
	FreeSignal((long) s->sigbit);
    if (s->whole_track)
	FreeMem(s->whole_track, s->tracklen);
    if (s->quest)
	FREE(s->quest);
    FlushTree(s->maybe);
    FlushTree(s->sausages);
    fent = s->parents;
    while (fent) {
	temp = fent->next;
	FlushList(&fent->ready);
	Freeblet(fent);
	fent = temp;
    }
    FREE(s);
}



long FastExamine(BPTR lok, fib *fibb)
{
    struct Process *meee = ThisProcess();
    register state *s = fibb->ss;
    bool sploft = false, deepe = (long) s == NEWDEEP || (long) s == FALLASDEEP,
			fall = (long) s <= FALLTHEWAY && (long) s >= FALLASYNCH,
			firsty = (long) s == NEWDEEP || (long) s == NEWSHALLOW;
    struct DeviceList *vawl = ValVol(gbip(lok), meee);
    pack *p = null, *np = (firsty ? null : (pack *) fibb->pard);
    long r;
    short sig = -1;

    if (!firsty)
	sploft = (fall ? vawl != gbip(bip(struct FileLock, np->lock)->fl_Volume)
			: s->vol != vawl);
    if (lok && !fall && !sploft) {
	if (!vawl)
	    return DOSFALSE;
	if (TrackExamine(lok, fibb, vawl, &deepe, firsty))
	    return DOSTRUE;
    }
    if (!sploft && ((long) fibb->ss > NEWSHALLOW
		    || (long) fibb->ss < FALLASYNCH))
	return DOSFALSE;
    fibb->ss = (adr) FALLTHEWAY;
    r = Examine(lok, (struct FileInfoBlock *) fibb);
    if (!lok || fibb->f.fib_EntryType < 0 || sploft) return r;
    if (np) sig = np->repp.mp_SigBit;		/* FALLASDEEP only */
    if (!~sig) sig = AllocSignal(-1L);
    if (~sig && r && ((pack *) fibb->pard = NEWPZ(p))) {
	fibb->ss = (adr) (deepe ? FALLASDEEP : FALLASYNCH);
	InitPort(&p->repp, sig);
	p->fibuf = fibb->f;
	p->isout = false;
	p->lock = lok;
	if (np) {
	    p->chain = np->chain;		/* so everybody gets freed */
	    np->chain = p;
	}
	SendExNext(p);
    } else {
	if (p) FREE(p);
	if (firsty && ~sig) FreeSignal((long) sig);
    }
    return r;
}



long FastExNext(BPTR lok, fib *fibb)
{
    struct FileLock *lick = gbip(lok);
    fiblet *gut;
    long loew, kee = lick->fl_Key;
    register state *s = fibb->ss;
    register fiblet *who = fibb->pard;
    struct Process *meee = ThisProcess();

    if ((long) s == CLEANED) {
	meee->pr_Result2 = ERROR_NO_MORE_ENTRIES;
	return DOSFALSE;
    }
    if ((long) s == FALLTHEWAY)
	return ExNext(lok, (struct FileInfoBlock *) fibb);
    if ((long) s == FALLASYNCH || (long) s == FALLASDEEP) {
	pack *p = (adr) fibb->pard;
	WaitExNext(p);
	fibb->f = p->fibuf;
	loew = p->sp.sp_Pkt.dp_Res1;
	if (!loew && (meee->pr_Result2 = p->sp.sp_Pkt.dp_Res2)
					== ERROR_NO_MORE_ENTRIES) {
	    if ((long) s != FALLASDEEP)
		FastExCleanup(fibb);
	} else SendExNext(p);		/* continue in spite of error */
	return loew;
    }
    if (!ValVol(lick, meee))
	return DOSFALSE;
    if (who != s->lastwho)
	Checkbies(who, s);
    while (!(gut = Pop(&who->ready))) {
	if (meee->pr_Result2 = who->length)
	    goto bomb;				/* DigestTrack found an error */
	if (!s->pending) {			/* MaybeAskTrack didn't */
	    if (loew = Nearest(s->mid_sector, who->wanted))
		AskTrack(loew, s, who);
	}
	if (!DigestTrack(s))
	    if (!(loew = Nearest(s->mid_sector, who->wanted)))
		who->length = ERROR_NO_MORE_ENTRIES;
	    else
		MaybeAskTrack(loew, s, who);
    }
    ChewSomeMore(gut, fibb);
    Freeblet(gut);
    s->lastwho = who;
    return DOSTRUE;
bomb:
    who->length = 0;
    if (!s->depth && meee->pr_Result2 != ERROR_NOT_A_DOS_DISK)
	FastExCleanup(fibb);
    return DOSFALSE;		/* allow continuation after hard error */
}



long FSRexxQuery(struct Message *rm)
{
    /* FOR NOW, just: */
    return 1;			/* ARexx "program not found" error code */
}


#asm
		cseg
endtag:		ds.w	0		; don't search for romtags until here
#endasm
