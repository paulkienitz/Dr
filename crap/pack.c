/* a leetle thingy to test packet weirdness */

#include <dos/dosextens.h>
#include <Paul.h>


int main(void)
{
    struct Process *me;
    struct InfoData *ind;
    adr cont;
    long r, i;

    me = (adr) FindTask(null);
    cont = bip(struct FileHandle, me->pr_COS)->fh_Type;
    if (cont && NEWP(ind)) {
	printf("the console msgport is at %lx.\n", cont);
	r = DoPkt1(cont, (long) ACTION_DISK_INFO, (ulong) ind >> 2);
	i = IoErr();
	FREE(ind);
	printf("It returned %ld, IoErr %ld.\n", r, i);
    } else
	puts("No msgport or no memory!");
    return 0;
}
