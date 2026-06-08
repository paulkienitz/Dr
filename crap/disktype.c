/* tells the dostype of a file's volume.  Use short ints. */

#include <dos/dosextens.h>
#include <Paul.h>

int main(int argc, str *argv)
{
    BPTR ll;
    struct FileLock *llp;
    struct DeviceList *vv;
    ulong dt;
    ustr dc = (adr) &dt;

    if (argc != 2) return 10;
    if (!(ll = RLock(argv[1]))) return 10;
    llp = gbip(ll);
    vv = gbip(llp->fl_Volume);
    Printf("The FileLock is at %lx; the volume is at %lx.\n", llp, vv);
    Printf("The volume name is \"%s\".\n\n", bip(char, vv->dl_Name) + 1);
    dt = vv->dl_DiskType;
    Printf("The DosType bytes are %02x %02x %02x %02x, or '",
		dc[0], dc[1], dc[2], dc[3]);
    Printf("%c%c%c%c'.\n", dc[0], dc[1], dc[2], dc[3]);
    return 0;
}
