/* This program is just like ForEach only different.  It's also completely
different from my hack Whichever, except it's mostly the same.  What it does
is read a file from standard input (use redirection) and a command string on
its argument line, and for each line of the input it executes the command
with the line of input substituted where it finds [] in the command.

An earlier version of this was supposed to have gone out with Dr 1.2, and
though the executable went out, the source got left out of the distribution
archive somehow, so the record of how that version differed has been lost.

By Paul Kienitz 12/90 public domain. */

/* modified 8/22/92 to use System() under dos 2.0, which has the effect of
making ctrl-D interrupt work a lot better.  Woops -- there was actually no
ctrl-D interrupt feature at all before ... so I added that too. */


#include <dos/dos.h>
#include <dos/dostags.h>
#include <Paul.h>

#define SIG_C_D (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D)


#define CHARLIMIT 255
#define CMDLIMIT 255
#define IBSIZE 1000


struct Library *DOSBase;

struct Buf {
    char buf[IBSIZE];
    short pos;
    short fill;
    char line[CHARLIMIT + 1];
};



void Spew(s) str s;
{
    register BPTR o = Output();
    if (o) Write(o, s, (long) strlen(s));
}



/* buffered read function; returns true when eof, result put in *c */
bool Read1(c, b) char *c; struct Buf *b;
{
    if (b->pos >= b->fill) {
	b->fill = Read(Input(), b->buf, (long) IBSIZE);
	b->pos = 0;
	if (b->fill <= 0) {
	    *c = 0;
	    return true;		/* eof */
	}
    }
    *c = b->buf[b->pos++];
    return false;
}



str NextLine(b) struct Buf *b;
{
    short ll;
    char c;

    do {
	ll = 0;
	while (!Read1(&c, b) && c != '\n' && c != '\f') {
	    b->line[ll++] = c;
	    if (ll > CHARLIMIT) {
		Spew("ForEvery:  Input line too long:  \"");
		strcpy(b->line + 30, "...\"\n");
		Spew(b->line);
		while (!Read1(&c, b) && c != '\n' && c != '\f') /* skip */ ;
		ll = 0;
	    }
	}
    } while (!ll && (c == '\n' || c == '\f'));		/* skip empty lines */
    b->line[ll] = 0;
    if (ll) return &b->line[0];
    else return null;			/* eof */
}



/* returns 0 for success, 1 for error, -1 for finished */

short Stubbitute(oldlen, old, new, insert) long oldlen; str old, new, insert;
{
    register str p = old;
    register short i = oldlen;

    if (!insert)
	return -1;					/* finished */
    if (old[i - 1] == '\n')				/* usually true */
	i--;
    if (i + strlen(insert) > CMDLIMIT + 2) {
	Spew("ForEvery:  Command too long with \"");
	i = strlen(insert);
	if (i > 30) 
	    strcpy(insert + 30, "...\"\n");
	else strcpy(insert + i, "\"\n");
	Spew(insert);
	*new = 0;
	return 0;
    }
    while (--i)
	if (*(p++) == '[' && *p == ']')
	    break;
    if (!i) {
	Spew("ForEvery:  Command arg must mark where to substitute with [].\n");
	return 1;
    }
    while (old < p)
	*(new++) = *(old++);
    new--;					/* un-copy the '[' */
    while (*insert)
	*(new++) = *(insert++);
    old++;					/* skip the marker */
    if (!i)
	return 0;
    while (--i)
	*(new++) = *(old++);
    *new = 0;
    return 0;
}



long _main(alen, aptr) long alen; str aptr;
{
    char command[CMDLIMIT + 1];
    struct Buf b;
    short r;
    BPTR out = Output();

    memset(&b, 0, sizeof(b));
    if (!Input()) {
	Spew("ForEvery:  No input file!\n");
	return 20;
    }
    while (!(r = 9, SetSignal(0L, (long) SIG_C_D) & SIG_C_D)
		    && !(r = Stubbitute(alen, aptr, command, NextLine(&b))))
	if (*command)
	    if (DOSBase->lib_Version >= 36)
		SystemTags(command, SYS_UserShell, (long) true, TAG_DONE);
	    else
		Execute(command, 0L, out);
    if (r == 9)
	return 5;
    else if (r > 0)
	return 10;
    else
	return 0;
}
