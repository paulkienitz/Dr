/*  PUREIO.C  */

#ifdef THIS_IS_A_COMMENT

Pureio is a set of vaguely stdio-ish functions for buffered writing to
standard output with reentrancy (in effect), for writing "pure" programs.  It
is efficient and quite compact.  It uses a strange hack to get access to an
address that you don't explicitly pass it with each call.  It is intended for
Aztec C programs, because I think Lattice C can get reentrant IO with the
amiga.lib stdio functions, if I'm not mistaken.

Pureio has two versions:  the normal one is synchronous and flushes output
each time it encounters a newline.  The buffer is generally not much more
than one line long in this case; 128 characters is typical.  The other is
asynchronous and flushes output only when the buffer is full or when
requested.  The buffer is often large in this case, like 500 to 5000
characters.  There are actually two buffers, each of the requested size, so
that one can be filling up while another is being written out.

To compile the asynchronous version of pureio, #define the symbol
ASYNCHRONICITY (e.g. with the -d option in the cc command).  Compiling this
file without that symbol produces the synchronous version.

The asynchronous version offers much less advantage for speed than you might
think.  In writing to CON: particularly, it will save little time, and that
only if the priority of the process using pureio is at least equal to the
priority of the CON handler, which is typically five; otherwise the CON
process hogs the CPU and your program doesn't procede worth a damn.  Even if
what you want to do simultaneously with the CON output is not calculation,
your program won't get far enough to start up that other operation until the
CON action is finished.  (Why doesn't console.device spend more time waiting
on the blitter?)  And it causes output to be uninterruptible by typein or
control-C for many lines at a time, if you use a large buffer.  Thus, the Dr
program, for which this was originally written, uses the synchronous version
now, because the asynchronous offered little improvement.  For the most
efficient overlapping of IO with other stuff, you might try putting a flush
(call the void function pflush()) just before whatever section of prolonged
calculation or disk IO or whatever that you want to have running
simultaneously with the writing of output.

----------------------------------------------------------------------------

To use pureio, you must allocate a variable containing eighteen short words
(28 bytes) in your outermost function invocation (equivalent of main).  IT
MUST BE IN SUCH A PLACE THAT THE POSITION IT OCCUPIES ON YOUR STACK NEVER
DIFFERS BETWEEN DIFFERENT RUNS OF YOUR PROGRAM, and be aligned.  It is
actually going to be an instance of the struct called pureioshit below, which
is 36 bytes in size.  Let's say you create that structure in a variable
called purestuff.  You must then initialize pureio by passing the address of
purestuff to the bool function OpenPureIO().  As a second argument, pass a
long integer value giving the size of the buffer that pureio will use, in
bytes.  An upper limit of 10000 bytes is enforced.  As a third arg, pass the
address of a function which is to be called whenever pureio flushes its
output.  This would often be the address of a function that checks whether
Control-C has been pressed.  Use NULL if you don't want such a function to be
called.  The fourth argument is an address that gets passed to this function.
Example of initializing pureio:

    _main()	/* avoid stdio startup code by using _main, not main */
    {
	short purbuf[14];
	void Chk_Abort();
	... other declarations ...
	if (InitPureIO(purbuf, 500L, Chk_Abort, NULL)) {
	    ...fail, exit;
	}
	... blah blah, you can use pureio functions now ...
	ClosePureIO();
    }

You have to call ClosePureIO before exiting.  The functions offered are all
void except OpenPureIO, which returns nonzero if there's insufficient memory.
You can declare it as either short or long.  They are named differently from
the nearest equivalent stdio functions.  You can #define the usual names to
be these if you want, but don't use those in places where you expect a return
value.  The functions are:

    BOOL OpenPureIO(short *block, long size, void (*ffunc)(void *ffa), void *ffa)
    void puch(short c)		/* like putchar(), writes one character     */
    void put(char *s)		/* like fputs(s, stdout), writes the string */
    void putfmt(char *fmt, ...)	/* a simple printf, based on RawDoFmt       */
    void pflush(void)		/* flushes buffered output                  */
    void ClosePureIO(void)	/* releases memory, etc; call this last     */

We make the argument to puch a short rather than a char just to save trouble.
The use of putfmt is just like regular printf, with the following exceptions.
1) Whether you use short or long ints, %d and %x and so on always refer to
shorts, and you have to use %ld etc for longs.  (If you use long ints, you'll
have to use %lc to print chars.)  2) There is no %*d feature to specify field
width with an int parm.  3) Dig this:  there is no %u!  Decimal output is
always signed.  4) There is no %o for octal.  5) There is no floating point
formatting of any kind; no %e, %f, or %g.  Left justified fields, padding with
leading zeroes, and maximum lengths for strings with e.g. %.47s are available.

Output is actually written whenever the number of unwritten characters
reaches the size of the buffer or whenever you call pflush.  (ClosePureIO
calls pflush.)  In addition, the synchronous version flushes output at the
end of each line.  The asynchronous version waits for the previous output
packet to return before sending out the current one (no double buffering). 
There is no provision for input in this version.  NOTE THAT when output is
written, it calls the function Chk_Abort() to see if control-C has been
pressed lately.  You must set the variable Enable_Abort to either zero or
non-zero only once at the beginning of your program to determine whether this
can call _abort().  Note that the default version of _abort calls _exit and
is not suitable for a pure program, or any that uses _main instead of main. 
Also, pureio will call _abort() directly if the standard output file handle
is null when it's time to output.

A note on using _main instead of main.  This makes your Manx program much
smaller, if you are willing to give up stdio functions (e.g. puts, printf,
anything that uses type FILE) and Un*#$%@!x-like io functions (e.g. open,
creat, etc), and use only raw AmigaDOS io funcs.  You can still use sprintf.
Also, if you want to be runnable from Workbench, you must explicitly wait for
the Workbench startup message, and reply it when you exit.  You cannot use
detach.o.  You must not call exit() or _exit(); use DOS Exit.  This means
that you cannot assume that any file will be automatically closed or that any
malloc'd memory will be automatically freed.  You cannot use the stack
checking option unless you do some tricky hacks.  Your argument line will not
be broken into argv-words, but will still be one string.  Also, the feature
to have a WINDOW= tooltype automatically create a CON: window for standard io
won't happen unless you explicitly call the _wb_parse function (see your
Aztec documentation for how to do that).  _wb_parse is pure.  If you use
any floating point libraries, you'll have to explicitly close them.  I think
they get opened atomatically when needed.  **** Except Aztec 5.0d fails to
open mathtrans.library when using FFP floats, I've found!  That's a bug.

You can declare your _main like this:
    long _main(alen, aptr) long alen; char *aptr;
aptr points to the command argument line.  IT IS NOT NULL TERMINATED.  alen
gives its length in bytes.  It probably has a newline at the end.  The long
value you return is the program's exit return code.  (That's why it's called
a return code; your program actually returns it just like a function.  The
CLI actually does call your program as a function.)  To parse the arg line
into argv-words, see the source to _cli_parse in the file crt_src/cliparse.c
on the Aztec C disk Aztec3: (version 5.0) or SYS2: (version 3.6); the version
they give expects _exit to free some memory, and is not pure.  It also
expects you to put quotes inside quoted strings with "" instead of *".  My Dr
program contains an example of an alternate parser that uses *" and is pure
and never mistakes "-opt" enclosed in quotes for a real option.

This was written in 7/89 for use in the directory listing program Dr, by Paul
Kienitz.  Compileable only with Manx.  ANSI-ized 12/90.

#endif THIS_IS_A_COMMENT

#include <exec/ports.h>
#include <exec/memory.h>
#include <libraries/dosextens.h>
#include <Paul.h>

#ifdef put
#undef put
#endif



typedef struct StandardPacket Spak;

typedef struct {
    short bufpos;
    bool packetout;
    char *buffer, *otherbuffer;
    long bufsize;
    Spak *pak;
    BPTR koss;
    struct MsgPort *meep;
    void (*CC)(adr cca);
    adr CCA;
} pureioshit;

private long boofset;

/* This is the offset in bytes between where my pr_ReturnAddr points (the
base of my stack) and the start of the buffer struct.  This value must be
constant for all process, though the address arrived at is different for
each one, being a position in that process's stack.  It's a long instead of
a short because that saves two instructions wherever it's used. */



#ifdef ASYNCHRONICITY

private void SendPak(register pureioshit *b)
{
    register struct DosPacket *p = &b->pak->sp_Pkt;
    register struct MsgPort *dest = bip(struct FileHandle, b->koss)->fh_Type;
    str temp;
    p->dp_Link = (adr) &b->pak->sp_Msg;
    p->dp_Port = b->meep;
    p->dp_Type = ACTION_WRITE;
    p->dp_Arg1 = b->koss;
    p->dp_Arg2 = (long) b->buffer;
    p->dp_Arg3 = (long) b->bufpos;
    b->pak->sp_Msg.mn_ReplyPort = b->meep;
    temp = b->buffer;
    b->buffer = b->otherbuffer;
    b->otherbuffer = temp;
    if (dest) {			/* don't send if koss = nil: */
	PutMsg(dest, b->pak);
	b->packetout = true;
    }
    b->bufpos = 0;
}



private void WaitPak(register pureioshit *b)
{
    register struct Message *m;
    
    while (b->packetout) {
	WaitPort(b->meep);
	while (m = GetMsg(b->meep))
	    if (m == &b->pak->sp_Msg) {
		b->packetout = false;		/* junk mail gets ignored */
		break;
	    }
    }
}

#endif



private void flushit(register pureioshit *b)
{
    void Chk_Abort(void);
#ifdef ASYNCHRONICITY
    if (b->packetout)
	WaitPak(b);
    if (b->bufpos)
	SendPak(b);
#else
    if (b->bufpos && b->koss)
	Write(b->koss, b->buffer, (long) b->bufpos);
    b->bufpos = 0;
#endif
    if (b->CC) b->CC(b->CCA);
}



void puch(short c)
{
    register pureioshit *b =
		(adr) (((str) ThisProcess()->pr_ReturnAddr) + boofset);
    import void _abort(void);

    if (!b->buffer || !b->koss) {
	_abort();
	return;
    }
    b->buffer[b->bufpos++] = (char) c;
#ifdef ASYNCHRONICITY
    if (b->bufpos >= b->bufsize)
#else
    if (c == '\n' || b->bufpos >= b->bufsize)
#endif
	flushit(b);
}



#asm
	; void putfmt(format, ...) str format; (whatever) items; ...

	public	_putfmt,_LVORawDoFmt,_geta4

put1:	ext.w	d0			; necessary
	beq.s	naw			; RawDoFmt sends trailing nul
	movem.l	d2/d3/a4/a6,-(sp)
	jsr	_geta4			; won't hurt large data programs
	move.w	d0,-(sp)
	bsr	_puch			; this is a glue routine, see
	addq	#2,sp
	movem.l	(sp)+,d2/d3/a4/a6
naw:	rts

_putfmt:
	movem.l	a2-a6,-(sp)	; play it safe
	move.l	24(sp),a0	; format
	lea	28(sp),a1	; data items
	lea	put1,a2		; putting func - no second parm, garbage in a3
	move.l	4,a6
	jsr	_LVORawDoFmt(a6)	; -522
	movem.l	(sp)+,a2-a6
	rts

#endasm



void put(register str s)
{
    while (*s) puch((short) *(s++));
}



void pflush()
{
    flushit((pureioshit *) (((str) ThisProcess()->pr_ReturnAddr) + boofset));
}



long OpenPureIO(register pureioshit *b, ulong s, void (*ffunc)(adr ffa), adr ffa)
{
    register struct Message *m;
    boofset = (str) b - (str) ThisProcess()->pr_ReturnAddr;	/* hack city */
    b->packetout = b->bufpos = 0;	/* set buffer to emptiness */
#ifdef ASYNCHRONICITY
    if (!(b->koss = Output()) || !(b->pak = NewP(Spak))) {
#else
    if (!(b->koss = Output())) {
#endif
	b->buffer = null;
	return (1L);
    }
    if (s > 10000) s = 10000;
    b->bufsize = s;
    if (!(b->buffer = Alloc(s))) {
#ifdef ASYNCHRONICITY
	Free(Spak, b->pak);
#endif
	return (1L);
    }
#ifdef ASYNCHRONICITY
    if (!(b->otherbuffer = Alloc(s))) {
	FreeMem(b->buffer, s);
	Free(Spak, b->pak);
	return (1L);
    }
    b->meep = CreatePort(null, 0L);
    m = &b->pak->sp_Msg;
    m->mn_Length = 68;
    m->mn_Node.ln_Type = NT_MESSAGE;
    m->mn_Node.ln_Pri = 0;
    m->mn_Node.ln_Name = (adr) &b->pak->sp_Pkt;
#endif
    b->CC = ffunc;
    b->CCA = ffa;
    return (0L);
}



void ClosePureIO(void)
{
    register pureioshit *b =
		(adr) (((str) ThisProcess()->pr_ReturnAddr) + boofset);
    if (b->buffer) {
	flushit(b);
#ifdef ASYNCHRONICITY
	WaitPak(b);
	DeletePort(b->meep);
	Free(Spak, b->pak);
	FreeMem(b->otherbuffer, b->bufsize);
#endif
	FreeMem(b->buffer, b->bufsize);
	b->buffer = null;
    }
}
