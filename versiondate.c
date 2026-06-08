/* Writes out today's date in the form "28.2.94", with quotes and newline, */
/* for use in creating programs' $VER: strings by including the output.    */
/* If a name and version number (e.g. "Foo 1.69") are given on the command */
/* line, it writes out the information as a two line C file like this:     */
/*   #define VERSION "1.69"                                                */
/*   char _versionstring[] = "\0$VER: Foo 1.69 (1.4.95)\n\r";              */
/* This allows version number control to be handled in the makefile rather */
/* than in the program source.  Version number must begin with a digit.    */


#include <time.h>
#include <ctype.h>
#include <stdio.h>


int main(int argc, char **argv)
{
    time_t t;
    struct tm *tt;

    time(&t);
    tt = localtime(&t);
    if (argc == 1)
	printf("\"%d.%d.%d\"\n", tt->tm_mday, tt->tm_mon + 1, tt->tm_year);
    else if (argc == 3 && isdigit(argv[2][0]))
	printf("#define VERSION \"%s\"\nchar _versionstring[] = \"\\0$VER:"
			" %s %s (%d.%d.%d)\\n\\r\";\n", argv[2], argv[1],
			argv[2], tt->tm_mday, tt->tm_mon + 1, tt->tm_year);
    else {
	printf("Usage: %s [name version]\n", argv[0]);
	return 10;
    }
    return 0;
}
