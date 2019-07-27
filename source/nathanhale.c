/* Turns on the amigados 2.x bit that makes it use the asterisk as a file     */
/* pattern wildcard.  Named after Nathan Hale, who said "I regret that I have */
/* but one asterisk for my country."                                          */

/* by Paul Kienitz, 15 January 92, public domain. */


#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <pragmas/exec_lib.h>
#include <clib/dos_protos.h>
#include <pragmas/dos_lib.h>


struct DosLibrary *DOSBase;


void spew(char *s)
{
    ULONG strlen();
    register BPTR o = Output();
    if (o) Write(o, s, strlen(s));
}


long _main()
{
    struct RootNode *rune = DOSBase->dl_Root;
    register long r = 0;
    struct Process *pp = (void *) FindTask(NULL);
    struct Message *WBenchMsg = NULL;
    BPTR wbcon = NULL;

    if (!pp->pr_CLI) {
	WaitPort(&pp->pr_MsgPort);
	WBenchMsg = GetMsg(&pp->pr_MsgPort);
	wbcon = Open("con:100/50/500/60/NathanHale", MODE_NEWFILE);
	if (!pp->pr_COS) pp->pr_COS = wbcon;
	spew("\n    ");
    }
    if (DOSBase->dl_lib.lib_Version < 36) {
        spew("Requires AmigaDOS 2.0 or newer.\n");
        return 20;
    }
    if ((rune->rn_Flags ^= RNF_WILDSTAR) & RNF_WILDSTAR)
	spew("ACTIVATING");
    else {
        spew("Turning OFF");
        r = 5;
    }
    spew(" the asterisk-wildcard flag.\n");
    if (wbcon) {
	Delay(75 + 5 * r);
	Close(wbcon);
    }
    if (WBenchMsg) {
	Forbid();
	ReplyMsg(WBenchMsg);
    }
    return r;
}
