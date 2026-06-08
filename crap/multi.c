/*   ListMulti.c ::::::::::
This program, which I believe should work, hangs forever if run twice
with the same argument.

The following demonstrates a problem with multiple assigns which I
can't find any solution to.  It is an excerpt from a larger program
which I can't get to work.  The thing is, it works perfectly the
first time it runs, but if you run it again, it hangs halfway
through.  As far as I can tell the system is being restored as as
possible to its original state after the run, but somehow it clearly
is actually not restored correctly.

The sequence of events is, basically, LockDosList(), FindDosEntry(),
DupLock(), UnLockDosList(), NameFromLock(), UnLock().  The function
which never returns the second time is, inexplicably, NameFromLock().

My system where this happened is a 3000 with KS 37.175 and WB 38.28.
Run this program as a command with one argument: any multiply
assigned name.  Like "ListMulti FONTS:" if you have FONTS: assigned
to more than one directory.  You can use it on any one assigned name
once, and it hangs up the second time you give the same name.
 */

#include <exec/libraries.h>
#include <dos/dosextens.h>

#define MULTILIMIT 20

struct Library *DOSBase;

int main(int argc, char **argv)
{
    char *ass, *end, abspath[200];

    if (DOSBase->lib_Version < 37)
	return 20;
    if (argc != 2) {
	Printf("Usage: %s <multiply_assigned_name>:\n", argv[0]);
	return 10;
    }
    ass = argv[1];
    end = ass + strlen(ass) - 1;
    if (*end == ':') {
	struct AssignList *al;
	struct DosList *dl;
	if (dl = LockDosList(LDF_ALL | LDF_READ)) {
	    *end = 0;
	    dl = FindDosEntry(dl, ass, LDF_ALL | LDF_READ);
	    *end = ':';
	    if (dl && dl->dol_Type == DLT_DIRECTORY) {
		short l, locount = 1;
		BPTR lox[MULTILIMIT];
		lox[0] = DupLock(dl->dol_Lock);
		al = dl->dol_misc.dol_assign.dol_List;
		while (al && locount < MULTILIMIT) {
		    lox[locount++] = DupLock(al->al_Lock);
		    al = al->al_Next;
		}
		UnLockDosList(LDF_ALL | LDF_READ);
		for (l = 0; l < locount; l++)
		    if (lox[l]) {
			if (NameFromLock(lox[l], abspath, 200L))
			    Printf("%s => %s\n", ass, abspath);
			else
			    PrintFault(IoErr(), " *** skipping one");
			UnLock(lox[l]);
		    } /* else... ?  lox[l] should never be null */
		return 0;
	    }
	    UnLockDosList(LDF_ALL | LDF_READ);
	}
    }
    Printf("The name %s is not a multiple assign.\n", ass);
    return 5;
}