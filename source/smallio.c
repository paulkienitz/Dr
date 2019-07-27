/*  SMALLIO.C  */

#ifdef THIS_IS_A_COMMENT

This is a stripped-down of my old PUREIO.C which I used to use for Dr ... but
Dr now uses my PURIFY.A startup code for purity, so now this is just a sort of
miniature stdio, with optional asynchronous background output if BACKGROUND is
defined.

The functions offered are all void except OpenSmallIO, which returns nonzero if
there's insufficient memory.  You can declare it as either short or long.  They
are named differently from the nearest equivalent stdio functions.  You can
#define the usual names to be these if you want, but don't use those in places
where you expect a return value.  The functions are:

    BOOL OpenSmallIO(void (*ffunc)(void))	/* call before anything else */
    void puch(ushort c)		/* like putchar(), writes one character      */
    void put(char *s)		/* like fputs(s, stdout), writes the string  */
    void putfmt(char *fmt, ...)	/* a simple printf, based on RawDoFmt        */
    void pflush(void)		/* flushes buffered output                   */
    void CloseSmallIO(void)	/* releases memory, etc; call this last      */

    void StopAllOutput(void)	/* what it says ... BACKGROUND version only  */

We make the argument to puch a short rather than a char just to save trouble.
The argument to OpenSmallIO is a function to be called to check for a control-C
break and handle it if present.  NULL is okay.

#endif THIS_IS_A_COMMENT


#include <exec/ports.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <Paul.h>

#ifdef put
#  undef put
#endif


#define BUFSIZE 300
char _smallIO_buffer[BUFSIZE];
short _smallIO_bufpos;

private BPTR koss;		/* our output filehandle */

private void (*CC)(void);

#ifdef BACKGROUND

#  define BBSIZE 8000

private str bigbuffer;
private volatile short bbegin, bbend;		/* equal when buffer empty */
private volatile bool pumping = false, dumping = false;

private struct Process *meee, *pumpproc;
private struct MsgPort *momport;
private short momportsig;

extern struct Library *DOSBase;



#asm
		public	___readA4msg

FindTask	equ	-294
WaitPort	equ	-384
GetMsg		equ	-372
pr_MsgPort	equ	92
ln_Name		equ	10

___readA4msg:	movem.l	a5/a6,-(sp)
		move.l	(4).w,a6
		sub.l	a1,a1
		jsr	FindTask(a6)
		move.l	d0,a5
		lea	pr_MsgPort(a5),a0
		jsr	WaitPort(a6)
		lea	pr_MsgPort(a5),a0
		jsr	GetMsg(a6)
		move.l	d0,a4
		move.l	ln_Name(a4),a4		; get parent's A4 value
		movem.l	(sp)+,a5/a6
		rts				; return the message gotten
#endasm

struct Message *__readA4msg(void);



private void BackgroundPump(void)
{
    struct Message *mom;
    struct Process *myself;
    struct Task *parent;
    long sigs;
    ustr line;
    short llen, newbeg;
    struct Library *DOSBase;

    mom = __readA4msg();		/* we can use global vars now */
    DOSBase = OpenL("dos");		/* play by the rules, eh */
    parent = mom->mn_ReplyPort->mp_SigTask;
    pumping = true;
    Signal(parent, SIGBREAKF_CTRL_F);
    for (;;) {
	sigs = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
	if (sigs & SIGBREAKF_CTRL_C) {
	    Signal(parent, SIGBREAKF_CTRL_C);	/* "quitting; you should too" */
	    break;
	}
	if (dumping)
	    break;
	while (bbegin != bbend && !dumping) {	/* only reference to bbend */
	    line = bigbuffer + bbegin;
	    llen = strlen(line);
	    if (llen)
		Write(koss, line, (long) llen);
	    newbeg = bbegin + llen + 1;
	    if (newbeg < BBSIZE)
		bbegin = newbeg;
	    else
		bbegin = 0;
	    Signal(parent, SIGBREAKF_CTRL_F);	/* "there's more room now" */
	    if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
		break;
	}
    }
    Forbid();
    pumping = false;
    bbegin = bbend;
    pumpproc = null;
    CloseLibrary(DOSBase);
    ReplyMsg(mom);
}



private void WaitForRoom(void)
{
    long sigs;

    Forbid();
    if (pumpproc) {
	sigs = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | bit(momportsig));
/* CAREFUL that removing momportsig here doesn't break something! */
	if (pumpproc && sigs & SIGBREAKF_CTRL_C)
	    Signal((adr) pumpproc, SIGBREAKF_CTRL_C);    /* "let's both quit" */
    }
    Permit();
}



private short Paste(short oldend, str what, short whatlen)
{
    short newend;

    if (whatlen)
	strncpy(bigbuffer + oldend, what, (size_t) whatlen);
    newend = oldend + whatlen;
    bigbuffer[newend++] = '\0';
    return newend;
}



void _smallIO_flushline(void)
{
    short space, nowbegin, newend, chunk;

    if (!_smallIO_bufpos)
	return;
    if (!pumping) {
	Write(koss, _smallIO_buffer, (long) _smallIO_bufpos);
	_smallIO_bufpos = 0;
	if (CC) CC();
	return;
    }
    for (;;) {
	nowbegin = bbegin;		/* snapshot */
	if (bbend < nowbegin)
	    space = nowbegin - bbend - 1;
	else
	    space = (nowbegin + BBSIZE - 2) - bbend;
	if (space >= _smallIO_bufpos)
	    break;
	WaitForRoom();
    }
    if (bbend < nowbegin || BBSIZE - bbend > _smallIO_bufpos)
	newend = Paste(bbend, _smallIO_buffer, _smallIO_bufpos);
    else {
	chunk = BBSIZE - bbend - 1;		/* may be zero */
	Paste(bbend, _smallIO_buffer, chunk);
	newend = Paste(0, _smallIO_buffer + chunk, _smallIO_bufpos - chunk);
    }
    if (newend >= BBSIZE)
	bbend = 0;
    else
	bbend = newend;
    _smallIO_bufpos = 0;
    meee->pr_Result2 = 0;
    Signal((adr) pumpproc, SIGBREAKF_CTRL_F);	/* "get to woik, you" */
    if (CC) CC();
}



void pflush(void)
{
    if (!pumping)
	return;
    _smallIO_flushline();
    while (bbend != bbegin)
	WaitForRoom();
}



void StopAllOutput(void)
{
    Forbid();
    dumping = true;
    if (pumpproc)
	Signal((adr) pumpproc, SIGBREAKF_CTRL_F); /* "got a surprise for you" */
    while (pumping)
	WaitForRoom();
    Permit();
}

#else /* !BACKGROUND */

void pflush(void)
{
    if (_smallIO_bufpos)
	Write(koss, _smallIO_buffer, (long) _smallIO_bufpos);
    _smallIO_bufpos = 0;
    if (CC) CC();
}

#  define _smallIO_flushline pflush

#endif /* BACKGROUND */



#ifdef C_PUCH

void puch(ushort c)
{
    if (c == '\n')
	while (_smallIO_bufpos && _smallIO_buffer[_smallIO_bufpos - 1] == ' ')
	    _smallIO_bufpos--;
    _smallIO_buffer[_smallIO_bufpos++] = (char) c;
    if (c == '\n' || _smallIO_bufpos >= BUFSIZE)
	_smallIO_flushline();
}

#else

void puch(ushort c);
#  asm
	public	_puch		; profiling says this is worth optimizing...
	cseg

c	equr	d0
bpos	equr	d1
buf	equr	a0
BUFSIZE	equ	300		; MAKE SURE THIS MATCHES the C definition!

_puch:	move.w	4(sp),c
	move.w	__smallIO_bufpos,bpos
	lea	__smallIO_buffer,buf
	cmp.b	#10,c		; newline?
	bne.s	allies
heil:	tst.w	bpos
	beq.s	allies
	cmp.b	#32,-1(buf,bpos.w)
	bne.s	allies
	dbra	bpos,heil	; decrements and always branches back
allies:	move.b	c,(buf,bpos.w)
	addq.w	#1,bpos
	move.w	bpos,__smallIO_bufpos
	cmp.b	#10,c
	beq.s	flshu
	cmp.w	#BUFSIZE,bpos
	blt.s	puxt
flshu:	bsr	__smallIO_flushline
puxt:	rts

#  endasm
#endif



void put(register str s)
{
    while (*s)
	puch(*(s++));
}



#asm
		public	___moma4
___moma4:	move.l	a4,d0
		rts
#endasm

adr __moma4(void);


private long tagz[] = {
    NP_Entry, 0L, NP_Priority, 0L, NP_Name, 0L, NP_Input, NULL, NP_Output, NULL,
    NP_Error, NULL, NP_StackSize, 2000L, NP_CopyVars, (long) FALSE, TAG_DONE
};

long OpenSmallIO(void (*ffunc)(void))
{
    register struct Message *mom;
    _smallIO_bufpos = 0;		/* set buffer to emptiness */
    if (!(koss = Output()))
	return 1;
    CC = ffunc;
#ifdef BACKGROUND
    meee = ThisProcess();
    if (bigbuffer = AllocP(BBSIZE)) {
	bbegin = bbend = 0;
	if (momport = CreateMsgPort()) {
	    if (NEWP(mom)) {
		long mypri = meee->pr_Task.tc_Node.ln_Pri;
		static ubyte name[] = "Dr output pump";

		mom->mn_Node.ln_Name = __moma4();
		mom->mn_ReplyPort = momport;
		momportsig = momport->mp_SigBit;
		tagz[1] = (long) BackgroundPump;
		tagz[3] = mypri;
		tagz[5] = (long) &name[0];
		if (pumpproc = CreateNewProc((adr) tagz)) {
		    PutMsg(&pumpproc->pr_MsgPort, mom);
		    while (!pumping && pumpproc)
			Wait(SIGBREAKF_CTRL_F | bit(momportsig));
		}
		if (pumpproc) 
		    return 0;
		FREE(mom);
	    }
	    DeleteMsgPort(momport);
	    momport = null;
	}
	FreeMem(bigbuffer, BBSIZE);
    }
    return 1;
#else
    return 0;
#endif
}



void CloseSmallIO(void)
{
    pflush();
#ifdef BACKGROUND
    if (momport) {
	struct Message *mom;
	StopAllOutput();
	WaitPort(momport);	/* BackgroundPump is gone when msg received */
	mom = GetMsg(momport);
	FREE(mom);
	DeleteMsgPort(momport);
    }
    if (bigbuffer)
	FreeMem(bigbuffer, BBSIZE);
#endif
}



#asm
	; void putfmt(format, ...) str format; (whatever) items; ...

	public	_putfmt,_LVORawDoFmt,_geta4

put1:	ext.w	d0		; necessary
	beq.s	naw		; RawDoFmt sends trailing nul
	movem.l	d2/d3/a4/a6,-(sp)
	FAR CODE
	jsr	_geta4		; won't hurt large data programs
	NEAR CODE		; won't hurt cause we're near end of source file
	move.w	d0,-(sp)
	bsr	_puch		; this is a glue routine, see
	addq	#2,sp
	movem.l	(sp)+,d2/d3/a4/a6
naw:	rts

RawDoFmt	equ	-522

_putfmt:
	movem.l	a2-a6,-(sp)	; play it safe
	move.l	24(sp),a0	; format
	lea	28(sp),a1	; data items
	lea	put1,a2		; putting func - no second parm, garbage in a3
	move.l	4,a6
	jsr	RawDoFmt(a6)
	movem.l	(sp)+,a2-a6
	rts

#endasm
