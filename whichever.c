/*
This command, "Whichever", takes a list of filenames (or anything) in its
standard input, picks one line of that input at random, and substitutes that
line into its argument line where it finds "[]", and executes the resulting
string as a command.  Use it whenever you want to use a program on a random
file or anything.  I use it to pick a random 8svx sound to use for a beep. 
Like this:    Whichever < beepses Installbeep []
where beepses contains a list of 8svx filenames, one per line.  You might
want to pipe the output of some directory lister program into the input.

Limitations:  It sucks the entire input into memory, so keep it small.  It
has a preset limit to the number of lines it can read (see the #define for
LINELIMIT) and will stop reading when it reaches this limit.  It ignores
blank lines and lines that would produce a command longer than CMDLIMIT
characters.  CMDLIMIT must be 255 or less due to limitations of the dos
Execute() function.

By Paul Kienitz 7/5/90 public domain.  Last update 12/30/90.
*/

#define LINELIMIT 1000
#define CMDLIMIT 255
#define IBSIZE 2000

#include <libraries/dos.h>
#include <Paul.h>


/* maybe I'll make it pure later but not today. */

str lineys[LINELIMIT];
short tot = 0;				/* number of lines in lineys */

char inputbuf[IBSIZE];			/* used by Read1 */
short ibpos = IBSIZE;
short ibfill = IBSIZE;



/* This here is an allocation efficiencizer which assumes that no
space is freed between allocations.  The size is in bytes.  The result is
shortword aligned.  I happened to have this in another program. */

str slablist = null;
#define slabsize 4000
ushort slabused = slabsize;



adr Shal(size) ushort size;
{
    adr t;
    void Spew();

    if (size & 1) size++;
    if (size > slabsize - slabused) {
	if (!(t = Alloc(slabsize))) {
	    Spew("Whichever: ran out of memory!\n");
	    return null;
	}
	*((str *) t) = slablist;   /* chain old onto new */
	slabused = 4;
	slablist = t;
    }
    t = slablist + slabused;
    slabused += size;
    return (t);
}



void FreeShalls()
{
    register str t;
    while (slablist) {
	t = slablist;
	slablist = *((str *) slablist);
	FreeMem(t, (long) slabsize);
    }
}



void Spew(s) str s;
{
    register BPTR o = Output();
    if (o) Write(o, s, (long) strlen(s));
}



/* buffered read function; returns true when eof, result put in *c */
bool Read1(c) char *c;
{
    if (ibpos >= ibfill) {
	ibfill = Read(Input(), inputbuf, (long) IBSIZE);
	ibpos = 0;
	if (ibfill <= 0) return true;		/* eof */
    }
    *c = inputbuf[ibpos++];
    return false;
}



void ReadLines(maxsub) short maxsub;
{
    bool eof;
    char linebuf[CMDLIMIT + 2];
    short lbpos;
    char c;

    do {
	lbpos = 0;
	while (!(eof = Read1(&c)) && c != '\n' && c != '\f')
	    if (lbpos <= maxsub)
		linebuf[lbpos++] = c;
	linebuf[lbpos] = 0;
	if (!(lineys[tot] = Shal(lbpos + 1)))
	    break;
	if (lbpos > 0 && lbpos <= maxsub) {
	    strcpy(lineys[tot], linebuf);
	    tot++;
	}
    } while (tot < LINELIMIT && !eof);
}



bool Stubbitute(oldlen, old, new, insert) short oldlen; str old, new, insert;
{
    register str p = old;
    register short i = oldlen;

    while (--i)
	if (*(p++) == '[' && *p == ']')
	    break;
    if (!i) {
	Spew("Whichever:  Command arg must mark where to substitute with [].\n");
	return true;
    }
    while (old < p)
	*(new++) = *(old++);
    new--;					/* un-copy the '[' */
    while (*insert)
	*(new++) = *(insert++);
    old++;					/* skip the marker */
    if (!i)
	return false;
    while (--i)
	*(new++) = *(old++);
    *new = 0;
    return false;
}



long _main(alen, aptr) long alen; str aptr;
{
    char command[CMDLIMIT + 1];
    long date[3];
    str which;
    short len = alen;

    if (!Input()) {
	Spew("Whichever:  No input file!\n");
	return 20;
    }
    if (aptr[len - 1] == '\n')			/* usually true */
	len--;
    ReadLines(CMDLIMIT + 2 - len);
    if (!tot) {
	Spew("Whichever:  no usable lines in input file!\n");
	FreeShalls();
	return 10;
    }
    DateStamp((adr) date);
    srand((int) (date[0] ^ date[1] ^ date[2]));
    which = lineys[rand() % tot];
    if (Stubbitute(len, aptr, command, which))
	return 10;
    Execute(command, 0, Output());
    FreeShalls();
    return 0;
}
