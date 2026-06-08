/* a simple little utility to measure how long another program takes to run.
Give a CLI command as the argument to this command.  Uses Execute() so make
Run resident and use SetPatch 1.3.2 or newer, or the times will come out
slower than they should. */


#include <devices/timer.h>
#include <string.h>

typedef struct {
    struct timerequest millie;
    struct timeval start;
} clok;

void *GfxBase, *TimerBase, *OldOpenLibrary(), CloseDevice(), CloseLibrary();
long Execute(), IoErr(), Output(), OpenDevice();

#define IORP struct IORequest *


void TimerBeginIO(IORP q), SubTime(struct timeval *dest, struct timeval *src);
#pragma amicall(TimerBase, 30, TimerBeginIO(a1))
#pragma amicall(TimerBase, 48, SubTime(a0, a1))



void Now(struct timerequest *millie, struct timeval *t)
{
    millie->tr_node.io_Command = TR_GETSYSTIME;
    millie->tr_node.io_Flags = IOF_QUICK;
    TimerBeginIO((IORP) millie);
    t->tv_secs = millie->tr_time.tv_secs;
    t->tv_micro = millie->tr_time.tv_micro + VBeamPos() * 63;
}

/* the VBeamPos is just a low-cost way of adding some extra precision */


BOOL TimerOn(clok *c)
{
    c->millie.tr_node.io_Message.mn_ReplyPort = (void *) -1;
    if (OpenDevice("timer.device", UNIT_VBLANK, (IORP) &c->millie, 0))
	return FALSE;
    TimerBase = c->millie.tr_node.io_Device;
    GfxBase = OldOpenLibrary("graphics.library");	/* can't fail */
    return TRUE;
}



void TimerOff(clok *c)
{
    CloseDevice((IORP) &c->millie);
    CloseLibrary(GfxBase);
}


#define StartStopwatch(c) Now(&(c)->millie, &(c)->start)

long CheckStopwatch(clok *c)	/* returns milliseconds since StartStopwatch */
{
    struct timeval now;
    Now(&c->millie, &now);
    SubTime(&now, &c->start);
    return now.tv_micro / 1000 + now.tv_secs * 1000;
}


void insal(char *buf, unsigned long val, char *tail, long leadz)
{
    short d = 11;
    char mini[12] = "00000000000";
    do {
	mini[--d] = val % 10 + '0';
	val /= 10;
    } while (val);
    for (d = 0; mini[d] == '0' && 11 - d > leadz; d++) ;
    buf += strlen(buf);
    strcpy(buf, mini + d);
    strcpy(buf + 11 - d, tail);
}


long _main(long alen, char *aptr)
{
    long r, e, t, o = Output();
    char buf[200];
    clok c;

    if (!TimerOn(&c)) {
	if (o) Write(o, "AARGH!  Can't open timer!\n", 26);
	return 20;
    }
    aptr[--alen] = 0;
    StartStopwatch(&c);
    r = Execute(aptr, 0, o);
    t = CheckStopwatch(&c);
    e = IoErr();
    if (!r && o) Write(o, "Execute function returned failure!\n", 35);
    if (e && o) {
	strcpy(buf, "Program set dos error code ");
	insal(buf, e, ".\n", 1);
	Write(o, buf, strlen(buf));
    }
    if (o) {
	strcpy(buf, "\nThat took ");
	insal(buf, t / 1000, ".", 1);
	insal(buf, t % 1000, " seconds to run.\n", 3);
	Write(o, buf, strlen(buf));
    }
    TimerOff(&c);
    return 0;
}
