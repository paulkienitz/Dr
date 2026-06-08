/* open a library (device?) named on the command line; expunge old libraries
first. */

#include <string.h>

long _main(long alen, char *aptr)
{
    void *base, *OpenLibrary();
    long Output(), o = Output();
    char buf[300];
    buf[0] = 0;
    AllocMem(999999999, 0);		/* expunge */
    strncpy(buf, aptr, alen);
    while (buf[alen - 1] <= ' ' && alen > 0) alen--;
    buf[alen] = 0;
    base = OpenLibrary(buf, 0);
    if (o) Write(o, buf, strlen(buf));
    if (base) {
	if (o) Write(o, " -- opened.\n", 12);
	CloseLibrary(base);
	return 0;
    } else {
	if (o) Write(o, " -- OpenLibrary failed!\n", 24);
	return 10;
    }
}
