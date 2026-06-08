/* For some reason the regular Delete command fails on soft links. */

#include <exec/libraries.h>
#include <clib/dos_protos.h>
#include <pragmas/dos_lib.h>

struct Library *DOSBase;

long _main(long alen, char *aptr)
{
    char *end = aptr + alen;

    if (DOSBase->lib_Version < 37)
	return 20;
    while (*--end == '\n' || *end == ' ' || *end == '\t')
	;
    end[1] = 0;
    while (*aptr == ' ' || *aptr == '\t')
	aptr++;
    if (*aptr == '"' && *end == '"')
	aptr++, *end = 0;
    if (!alen) {
	PutStr("Usage: UNLINK pathname\n");
	return 20;
    }
    if (DeleteFile(aptr)) {
	PutStr(aptr);
	PutStr(" deleted.\n");
	return 0;
    }
    PutStr("Could not delete ");
    PrintFault(IoErr(), aptr);
    return 10;
}
