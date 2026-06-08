/* trying out the ErrorReport function */


#include <dos/dosextens.h>
#include <Paul.h>


void main()
{
    BPTR l = RLock("sys:");
    ErrorReport(ABORT_DISK_ERROR, REPORT_LOCK, l, (adr) 23);
    UnLock(l);
}
