#include <stdio.h>
#include <Xm/Xm.h>
#include "aplot.h"
#include "version.h"

extern Widget toplevel;
extern XtAppContext app_con;

#ifdef VMS
#include <libdef.h>
#include <descrip.h>
#include <rms.h>
static void replacewildcards_VMS(int ac, char *av[], int *newac, char ***newav);
static void freewildcards_VMS(int newac, char **newargv);
#endif /*VMS*/

int main(int ac, char *av[])
{
	int i, argc;
	char **argv;

#ifdef VMS
	replacewildcards_VMS(ac, av, &argc, &argv);
#else /*VMS*/
	argc = ac;
	argv = av;
#endif /*VMS*/

	for (i=1; i < ac; i++)
	{
		if ((0 == strcmp(av[i], "-version")) ||
			(0 == strcmp(av[i], "-v")))
		{
			fprintf(stderr, "aplot build %d version %d\nWritten by Amish S. Dave.\n", APLOT_BUILD, APLOT_VERSION);
			return 0;
		}
		else if ((0 == strcmp(av[i], "-help")) ||
			(0 == strcmp(av[i], "-h")))
		{
			fprintf(stderr, "\n");
			fprintf(stderr, "aplot accepts the following arguments:\n");
			fprintf(stderr, "  aplot [-h] [-v] [-grouplbl] [-dottype] [-seq2] [-def] <files>\n");
			fprintf(stderr, "\n");
			fprintf(stderr, "\t[-h]         optional, returns this help message\n");
			fprintf(stderr, "\t[-v]         optional, returns aplot version\n");
			fprintf(stderr, "\t[-grouplbl]  optional, in postscript output, controls whether\n");
			fprintf(stderr, "\t               each plot gets an identifying label (filename)\n");
			fprintf(stderr, "\t               or each group\n");
			fprintf(stderr, "\t[-dottype dot/halfdash/dash]\n");
			fprintf(stderr, "\t             optional, controls appearance of raster marks in\n");
			fprintf(stderr, "\t               postscript output\n");
			fprintf(stderr, "\t[-seq2]      optional, whether saving in the osc or sono plot\n");
			fprintf(stderr, "\t               saves into .pcm or .pcm_seq2 files\n");
			fprintf(stderr, "\t[-def #]     optional, which defaults to use\n");
			fprintf(stderr, "\t[-set param=value]\n");
			fprintf(stderr, "\t             optional, set parameter to value\n");
			fprintf(stderr, "\t<files>      optional, can also load files after aplot has started\n");
			return 0;
		}
	}
	init_ui(argc, argv);

	XtRealizeWidget(toplevel);
	XtAppMainLoop(app_con);

#ifdef VMS
	freewildcards_VMS(argc, argv);
#endif /*VMS*/

	return 1;
}

void flush_xevents(void)
{
	XEvent event;

	while (XtAppPending(app_con))
	{
		XtAppNextEvent(app_con, &event);
		XtDispatchEvent(&event);
	}
}

#ifdef VMS
#define MAXFILES 512

void replacewildcards_VMS(int ac, char *av[], int *newac, char ***newav)
{
	long status;
	int i, cnt;
	struct dsc$descriptor_s filespec;
	struct dsc$descriptor_vs resultant;
	long context, findfilestatus, rmsstatus, flags = 2;
	char storage[128], **argv;
	unsigned short reslen;

	argv = (char **)malloc(MAXFILES * sizeof(char *));
	argv[0] = av[0];
	cnt = 1;

	filespec.dsc$b_dtype = DSC$K_DTYPE_T;
	filespec.dsc$b_class = DSC$K_CLASS_S;
	resultant.dsc$w_maxstrlen = sizeof(storage);
	resultant.dsc$b_dtype = DSC$K_DTYPE_VT;
	resultant.dsc$b_class = DSC$K_CLASS_VS;
	resultant.dsc$a_pointer = storage;

	for (i=1; i < ac; i++)
	{
		filespec.dsc$w_length = strlen(av[i]);
		filespec.dsc$a_pointer = av[i];
		for (;;)
		{
			findfilestatus = lib$find_file(&filespec, &resultant, &context,
				NULL, NULL, &rmsstatus, &flags);
			if (findfilestatus == RMS$_NORMAL)
			{
				reslen = *((unsigned short *)(resultant.dsc$a_pointer));
				argv[cnt] = (char *)malloc((reslen + 1) * sizeof(char));
				memcpy(argv[cnt], sizeof(unsigned short) + resultant.dsc$a_pointer, reslen);
				(argv[cnt])[reslen] = '\0';
				cnt++;
				if (cnt >= MAXFILES)
				{
					fprintf(stderr, "ERROR: too many files specified.  truncating. maximum = %ld\n", MAXFILES);
					cnt--;
					break;
				}
				continue;
			}
			break;
		}
		lib$find_file_end(&context);
	}
	*newac = cnt;
	*newav = argv;
	return;
}

void freewildcards_VMS(int newac, char **newargv)
{
	int i;

	for (i=1; i < newac; i++)
		free(newargv[i]);
	free(newargv);
}

#endif /*VMS*/

