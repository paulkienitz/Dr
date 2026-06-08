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


long _main()
{
    struct RootNode *rune = DOSBase->dl_Root;
    register long r = 0;
    struct Process *pp = (void *) FindTask(NULL);
    struct Message *WBenchMsg = NULL;
    BPTR wbcon = NULL, out = pp->pr_COS;

    if (!pp->pr_CLI) {
	WaitPort(&pp->pr_MsgPort);
	WBenchMsg = GetMsg(&pp->pr_MsgPort);
	wbcon = Open("con:100/50/500/60/NathanHale", MODE_NEWFILE);
	if (!out)
	    pp->pr_COS = out = wbcon;
	if (out) Write(out, "\n    ", 5);
    }
    if (DOSBase->dl_lib.lib_Version < 36) {
        if (out) Write(out, "Requires AmigaDOS 2.0 or newer.\n", 32);
        return 20;
    }
    if ((rune->rn_Flags ^= RNF_WILDSTAR) & RNF_WILDSTAR)
	Write(out, "ACTIVATING the asterisk-wildcard flag.\n", 39);
    else {
        Write(out, "Turning OFF the asterisk-wildcard flag.\n", 40);
        r = 5;
    }
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
