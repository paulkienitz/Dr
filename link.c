/* this here is sort of a MakeLink substitute */

#include <clib/dos_protos.h>
#include <pragmas/dos_lib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <paul.h>


struct Library *DOSBase;


void main(int ac, str *av)
{
    bool softness = ac == 4 && !stricmp(av[3], "SOFT");
    BPTR oldlock;
    long z, zz;

    if (ac != 3 && !(ac == 4 && softness)) {
	printf("Usage:  %s <newname> <oldname> [SOFT]\n\nThis program "
		"makes a hard or soft link from <newname> to <oldname>.\n", *av);
	exit(10);
    }
    if (DOSBase->lib_Version < 36) {
	puts("Requires AmigaDOS 2.0 or newer.");
	exit(20);
    }
    if (!(oldlock = RLock(av[2]))) {
	printf("Couldn't find \"%s\" to attach a link to.\n", av[2]);
	exit(10);
    }
    z = MakeLink(av[1], (softness ? (long) av[2] : oldlock), (long) softness);
    zz = IoErr();
    if (!z) {
	printf("\nMakeLink returned %ld, IoErr returned %ld.\n", z, zz);
	if (zz)
	    PrintFault(zz, "That error number means");
    }
    UnLock(oldlock);
}
