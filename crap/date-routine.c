short days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
char datefmt[] = "%02d %.3s %04d  %02d:%02d:%02d";

#define LEAP(y)    (!((y)%4)&&(((y)%100)||!((y)%400))) /* :-) */

void stamp_to_str(struct DateStamp *stamp,char *string)
{
    long xyzzy;
    short year,day,*d,hour,min,sec;
    char *month = months;

    xyzzy = (stamp->ds_Days + 1) * 100;
    year = (xyzzy / 36525) + 1978;
    day = (xyzzy % 36525 + 50) / 100;

    days[1] = 28 + LEAP(year);    /* fudge factor */

    for (d = days ; *d < day ; d++)
    {
        day -= *d;
        month += 3;
    }

    hour = stamp->ds_Minute / 60;
    min = stamp->ds_Minute % 60;
    sec = stamp->ds_Tick / TICKS_PER_SECOND;

    raw_do_fmt(string,datefmt,day,month,year,hour,min,sec);
}
