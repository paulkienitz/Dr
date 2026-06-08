/* USE 2.0 DEVICE LIST FUNCTIONS! */

#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <libraries/filehandler.h>
#include <Paul.h>


struct DosLibrary *DOSBase;
struct ExecBase *SysBase;

typedef struct DeviceNode den;
typedef struct FileSysStartupMsg funnies;


str envnames[19] = {
    "(size)",
    "Block size in lwords:    ",
    "SecOrg (not used):       ",
    "Number of heads:         ",
    "Sectors per block:       ",
    "Blocks per track:        ",
    "Reserved blocks:         ",
    "Prefac (not used):       ",
    "Interleave:              ",
    "Low cylinder:            ",
    "High cylinder:           ",
    "Number of buffers:       ",
    "Buffer memory type:      ",
    "Max bytes per transfer:  ",
    "Forbidden address mask:  ",
    "Autoboot priority:       ",
    "DOS type:                ",
    "????:                    ",
    "????:                    "
};


char hex[16] = "0123456789ABCDEF";


ulong rombase, romtop;


void FindRomRange(void)
{
    adr *rezs = SysBase->ResModules;
    ulong a, rez;

    rombase = ~0L;
    romtop = 0;
    Disable();
    while (rez = (ulong) *(rezs++)) {
	a = rez & 0xFFFC0000;		    /* round down to 256K boundary */
	if (a < rombase) rombase = a;
	a = (rez + 0x3FFFF) & 0xFFFC0000;   /* round up to 256K boundary */
	if (a > romtop) romtop = a;
    }
    Enable();
}



bool Valid(a) void *a;
{
    return TypeOfMem(a) || ((ulong) a >= rombase && (ulong) a < romtop);
}



str beast(b) BSTR b;
{
    ubyte *malloc(), *r, *s = bip(ubyte, b);
    ushort l, x;

    if (!s)
	return " <null> ";
    l = s[0];
    x = s[l + 1];
    if (!Valid(s))
	return " <Invalid Pointer> ";
    if (!(r = malloc(l + 4)))
	return " <Could not allocate copy!> ";
    strncpy(r, (str) s + 1, (size_t) l);
    r[l] = 0;
    return (str) r;
}



str Okay(b) void *b;
{
    if (!b || !~((long) b | 3)) return "";
    if (!Valid(b)) return "<INVALID!>";
    else return "(valid)";
}



bool SameName(a, b) str a, b;
{
    while (*a && toupper(*a) == toupper(*b))
	a++, b++;
    return toupper(*a) == toupper(*b);
}



void main(argc, argv) int argc; str *argv;
{
    den *d, *duuh;
    funnies *fun;
    long *envy, x;
    UBYTE *c;
    str a, nam, vice;
    int l;

    if (a = argv[1])
	l = strlen(a) - 1;
    if (argc != 2 || l < 0) {
	printf(
"\nUsage:  %s <drivename>\nwhere <drivename> is the DOS name of a disk\n"
"drive or partition, such as DH0:, not the name of a\n"
"disk volume or an assigned name.  The final colon is optional.\n", *argv);
	exit(10);
    }
    if (a[l] == ':')
	a[l] = 0;
    FindRomRange();
    Forbid();
    duuh = bip(den, bip(struct DosInfo, 
		((struct RootNode *) DOSBase->dl_Root) ->rn_Info
	   )->di_DevInfo);
    for (d = duuh; d; d = bip(den, d->dn_Next)) {
	if (SameName(nam = beast(d->dn_Name), a))
	    break;
    }
    Permit();
    if (!d) {
	printf("\nYou have no drive or partition called \"%s:\".\n", a);
	exit(10);
    }
    printf("\nHere is the info on device \"%s:\"  - - -\n\n", nam);
    if (d->dn_Type == DLT_VOLUME) {
	puts("It's a disk volume.  Please specify a dos device name instead.");
	exit(5);
    }
    if (d->dn_Type == DLT_DIRECTORY || d->dn_Type == DLT_LATE
				|| d->dn_Type == DLT_NONBINDING) {
	puts(
"It's an assigned name.  Please specify a dos device name instead.");
	exit(5);
    }
    if (d->dn_Type != DLT_DEVICE) {
	printf("It has a bogus number in the Type field: %ld!\n", d->dn_Type);
	exit(5);
    }
    printf("Handler message port:       %lx   %s\n",
		d->dn_Task, Okay(d->dn_Task));
    printf("Assigned directory lock:    %lx\n", d->dn_Lock);
    printf("Handler code filename:      %lx -> \"%s\"\n",
		d->dn_Handler, beast(d->dn_Handler));
    x = d->dn_SegList;
    printf("Handler code seglist:       %lx << 2 = %lx   %s\n",
		x, bip(long, x), Okay(bip(long, x)));
    printf("Stack size:                 %ld\n", d->dn_StackSize);
    printf("Task priority:              %ld\n", d->dn_Priority);
    x = d->dn_GlobalVec;
    printf("Global Vector:              %lx << 2 = %lx   %s\n",
		x, bip(long, x), Okay(bip(long, x)));
    fun = bip(funnies, d->dn_Startup);
    printf("Startup pointer:            %lx   %s\n", fun, Okay(fun));
    if (!Valid(fun))
	exit(0);
    vice = gbip(fun->fssm_Device);
    envy = gbip(fun->fssm_Environ);
    if (Valid(vice) && (!envy || Valid(envy))) {
	printf("\nThe startup values are:\n"
	       "   Flags:              %lx\n", fun->fssm_Flags);
	printf("   Unit number:        %ld\n", fun->fssm_Unit);
	printf("   Device driver:      %lx -> \"%s\"\n",
			fun->fssm_Device, beast(fun->fssm_Device));
	printf("   Environment:        %lx << 2 = %lx   %s\n",
			fun->fssm_Environ, envy, Okay(envy));
	if (Valid(envy)) {
	    x = envy[0];
	    printf("\nThe environment is size %ld, containing:\n", x);
	    for (l = 1; l <= x && l <= 18; l++) {
		if (l == DE_MASK)
		    printf("      %s %lx\n", envnames[DE_MASK], envy[DE_MASK]);
		else if (l == DE_DOSTYPE) {
		    printf("      %s %lx = '", envnames[DE_DOSTYPE],
				envy[DE_DOSTYPE]);
		    for (c = (UBYTE *) (envy + DE_DOSTYPE);
				c < (UBYTE *) (envy + DE_DOSTYPE + 1); c++)
			if (*c & 0x60)
			    putchar(*c);
			else {
			    putchar('^');
			    putchar(*c + 64);
			}
		    printf("'.\n");
		} else
		    printf("      %s %ld\n", envnames[l], envy[l]);
	    }
	}
    } else
	printf("\nThe startup message may be a BSTR:\n     \"%s\"\n",
			beast(fun->fssm_Environ));
    exit(0);
}



void _wb_parse()  {  }
