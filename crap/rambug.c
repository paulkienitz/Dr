/*
Why are soft links still not supported by the MakeLink command?  Could
it be because they still crash sometimes?

They seem to work just fine with the V37 ROM filesystem ... but in RAM:
they are buggy.  The V37 Ram-Handler likes to make enforcer hits when
you call ReadLink() on one of its soft links.  First it reads from
address -4, and depending on whether Enforcer is running and whether
it's in "deadly" mode, it may either return the correct string with
perhaps some extra garbage appended, return a failure, or destroy things
and force you to reboot.  It works fine with the disk filesystem.

Is this fixed in V39?

The following demonstrates buggy behavior, at least on my 3000 with
KS 37.175 and WB 38.28 ... run it with no arguments:
 */

#include <exec/memory.h>
#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

struct Library *DOSBase;

char linkfrom[] = "RAM:ThisIsASoftLink";
char linkname[288] = "SYS:";                 /* any pathname will do */

int main(void)
{
    char line[6];
    BPTR ramlok;
    struct FileInfoBlock *fib;
    struct MsgPort *handler;

    if (DOSBase->lib_Version < 37)
        return 20;
    PutStr("\nTHIS PROGRAM MAY CAUSE ENFORCER HITS OR EVEN A CRASH.\n");
    PutStr("DO NOT CONTINUE UNLESS YOU ARE READY FOR THIS TO HAPPEN.\n");
    PutStr("\nIf you want to go ahead and crash, type \"yes\" here: ");
    Flush(Output());
    FGets(Output(), line, 5);
    if (stricmp(line, "yes\n"))
        return 5;

    MakeLink(linkfrom, (long) linkname, TRUE);

    if (ramlok = Lock("RAM:", ACCESS_READ)) {
        handler = ((struct FileLock *) BADDR(ramlok))->fl_Task;
        if (fib = AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC)) {
            if (Examine(ramlok, fib))
                while (ExNext(ramlok, fib)) {
                    PutStr(fib->fib_FileName);
                    if (fib->fib_DirEntryType == ST_SOFTLINK
/* THIS is where it messes up: */        && ReadLink(handler, ramlok,
                                                     fib->fib_FileName,
                                                     linkname, 288))
                        Printf(" -- a soft link to %s!\n", linkname);
                    else
                        PutStr("\n");
                }
            FreeMem(fib, sizeof(struct FileInfoBlock));
        }
        UnLock(ramlok);
    }
    DeleteFile(linkfrom);
    return 0;
}
