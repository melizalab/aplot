#include <stdio.h>

#ifdef sun
#define UNIX
#else
#define VMS
#endif

char *mybasename(char *pathname)
{
	char *basename;

#ifdef UNIX
		/*
		** Algorithm for getting basename: find last '/' character, and start w/ next char.
		** If '/' not found, use whole name.
		*/
		basename = pathname;
		while ((*basename) && (*basename != '/')) basename++;
		if (*basename == '\0')									/* No '/' characters */
			basename = pathname;
		else
		{
			while (*basename) basename++;						/* Scan to end, then go backwards */
			while (*basename != '/') basename--;
			basename++;
		}
#else
#ifdef VMS
		/*
		** Algorithm for getting basename: find ']' character, and start w/ next char.
		** If ']' not found, repeat for ':'. If still not found, use whole name.
		*/
		basename = pathname;
		while ((*basename) && (*basename != ']')) basename++;
		if (*basename == '\0')									/* No '/' characters */
		{
			basename = pathname;
			while ((*basename) && (*basename != ':')) basename++;
			if (*basename == '\0') basename = pathname;
			else basename++;
		}
		else basename++;
#endif
#endif

	return basename;
}

