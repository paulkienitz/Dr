/* find or create a subdirectory called 300 in the current directory, and put
300 randomly named files in it. */

#include <libraries/dosextens.h>
#include <Paul.h>

long _main()
{
    char name[32], num[5] = "  0\r";
    short rand();
    struct DateStamp d;
    BPTR three, oldee, weee;
    long air = 0;
    struct Process *me = ThisProcess();
    short f, c;

    DateStamp((long *) &d);
    srand(d.ds_Days + d.ds_Minute + d.ds_Tick);
    three = CreateDir((ustr) "300");	/* why it uses DIRECTORY_NOT_EMPTY */
    if (!three) {			/*    instead of OBJECT_EXISTS is  */
	air = me->pr_Result2;		/*    beyond me ...                */
	if (air == ERROR_DIRECTORY_NOT_EMPTY || ERROR_OBJECT_EXISTS) {
	    three = RLock("300");
	    air = me->pr_Result2;
	    if (!three) {
		Write(me->pr_COS, "SAY WHAT?!?\n", 12);
		me->pr_Result2 = air;
		return 20;
	    } else air = 0;
	}  else {
	    Write(me->pr_COS, "Can't create subdir!\n", 21);
	    me->pr_Result2 = air;
	    return 20;
	}
    }
    oldee = CurrentDir(three);
    for (f = 0; f < 300; f++) {
	for (c = 0; c < 24; c++)
	    name[c] = 'A' + rand() % 60;
	name[c] = 0;
	if (!(weee = NOpen(name))) {
	    air = me->pr_Result2;
	    Write(me->pr_COS, "AAARRGH!!  Couldn't create file!\n", 33);
	    break;
	}
	Close(weee);
	if (!(f % 10) && f) {
	    num[2] = '0' + f % 10;
	    if (f >= 10) num[1] = '0' + (f / 10) % 10;
	    if (f >= 100) num[0] = '0' + f / 100;
	    Write(me->pr_COS, num, 5);
	}
    }
    CurrentDir(oldee);
    UnLock(three);
    Write(me->pr_COS, "Done.\n", 6);
    me->pr_Result2 = air;
    return air ? 10 : 0;
}
