

/**********************************************************************************
 * INCLUDES
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aplot.h"
#include "aplot_defaults.h"


/**********************************************************************************
 * GLOBAL VARS
 **********************************************************************************/
struct defaultsdata defaults;


/**********************************************************************************
 * PRIVATE TYPES
 **********************************************************************************/
typedef struct
{
	char *key;
	char *type;
	void *tgt;
} DEFSTRUCT;

static DEFSTRUCT defdata[] =
{
	{"xmin",			"long",		&defaults.xmin},
	{"xmax",			"long",		&defaults.xmax},
	{"ymin",			"long",		&defaults.ymin},
	{"ymax",			"long",		&defaults.ymax},
	{"gsmin",			"long",		&defaults.gsmin},
	{"gsmax",			"long",		&defaults.gsmax},
	{"frmin",			"long",		&defaults.frmin},
	{"frmax",			"long",		&defaults.frmax},
	{"xrangemin",		"long",		&defaults.xrangemin},
	{"xrangemax",		"long",		&defaults.xrangemax},
	{"xpos",			"int",		&defaults.xpos},
	{"width",			"int",		&defaults.width},
	{"zoomfactor",		"float",	&defaults.zoomfactor},
	{"outputwav",		"boolean",	&defaults.outputwav},
	{"showsliders",		"boolean",	&defaults.showsliders},
	{"usealsa",			"boolean",	&defaults.usealsa},

	{"sono_selected",	"boolean",	&defaults.sono_selected},
	{"sono_height",		"int",		&defaults.sono_height},
	{"sono_xaxis",		"boolean",	&defaults.sono_drawxaxis},
	{"sono_yaxis",		"boolean",	&defaults.sono_drawyaxis},
	{"fftsize",			"int",		&defaults.fftsize},
	{"fftolap",			"int",		&defaults.fftolap},
	{"method",			"int",		&defaults.method},
	{"scale",			"int",		&defaults.scale},
	{"windowtype",		"int",		&defaults.windowtype},
	{"npoles",			"int",		&defaults.npoles},
	{"colormap",		"int",		&defaults.colormap},

	{"osc_selected",	"boolean",	&defaults.osc_selected},
	{"osc_height",		"int",		&defaults.osc_height},
	{"osc_xaxis",		"boolean",	&defaults.osc_drawxaxis},
	{"osc_yaxis",		"boolean",	&defaults.osc_drawyaxis},

	{"label_selected",	"boolean",	&defaults.label_selected},
	{"label_height",	"int",		&defaults.label_height},
	{"label_xaxis",		"boolean",	&defaults.label_drawxaxis},
	{"label_yaxis",		"boolean",	&defaults.label_drawyaxis},

	{"psth_selected",	"boolean",	&defaults.psth_selected},
	{"psth_height",		"int",		&defaults.psth_height},
	{"psth_xaxis",		"boolean",	&defaults.psth_drawxaxis},
	{"psth_yaxis",		"boolean",	&defaults.psth_drawyaxis},
	{"psth_bindur",		"float",	&defaults.psth_bindur},
	{"psth_ymax",		"float",	&defaults.psth_ymax},

	{"rast_selected",	"boolean",	&defaults.rast_selected},
	{"rast_height",		"int",		&defaults.rast_height},
	{"rast_xaxis",		"boolean",	&defaults.rast_drawxaxis},
	{"rast_yaxis",		"boolean",	&defaults.rast_drawyaxis},
	{"rast_autoysize",	"boolean",	&defaults.rast_autoysize},
	{"rast_dotshape",	"int",		&defaults.rast_dotshape},

	{"isi_selected",	"boolean",	&defaults.isi_selected},
	{"isi_height",		"int",		&defaults.isi_height},
	{"isi_xaxis",		"boolean",	&defaults.isi_drawxaxis},
	{"isi_yaxis",		"boolean",	&defaults.isi_drawyaxis},
	{"isi_xrange",		"float",	&defaults.isi_xrange},
	{"isi_bindur",		"float",	&defaults.isi_bindur},

	{"video_selected",	"boolean",	&defaults.video_selected},
	{"video_height",	"int",		&defaults.video_height},

	{"psip_selected",	"boolean",	&defaults.psip_selected},
	{"psip_height",		"int",		&defaults.psip_height},
	{"psip_xaxis",		"boolean",	&defaults.psip_drawxaxis},
	{"psip_yaxis",		"boolean",	&defaults.psip_drawyaxis},

	{"ccor_selected",	"boolean",	&defaults.ccor_selected},
	{"ccor_height",		"int",		&defaults.ccor_height},
	{"ccor_xaxis",		"boolean",	&defaults.ccor_drawxaxis},
	{"ccor_yaxis",		"boolean",	&defaults.ccor_drawyaxis},
	{"ccor_wsize",		"float",	&defaults.ccor_wsize},
	{"ccor_tau",		"float",	&defaults.ccor_tau},
	{"ccor_taustep",	"float",	&defaults.ccor_taustep},

	{NULL, NULL, NULL},
};


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
void init_defaults(void);
int set_default(char *key, char *value);
void load_defaults(int defsect);
void save_defaults(int defsect);


/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
void init_defaults(void)
{
	defaults.sono_selected = TRUE;
	defaults.osc_selected = TRUE;
	defaults.label_selected = FALSE;
	defaults.psth_selected = FALSE;
	defaults.rast_selected = FALSE;
	defaults.psip_selected = FALSE;
	defaults.video_selected = FALSE;
	defaults.ccor_selected = FALSE;
	defaults.isi_selected = FALSE;

	defaults.sono_drawxaxis = TRUE;
	defaults.sono_drawyaxis = TRUE;
	defaults.osc_drawxaxis = TRUE;
	defaults.osc_drawyaxis = TRUE;
	defaults.label_drawxaxis = TRUE;
	defaults.label_drawyaxis = TRUE;
	defaults.psth_drawxaxis = TRUE;
	defaults.psth_drawyaxis = TRUE;
	defaults.rast_drawxaxis = TRUE;
	defaults.rast_drawyaxis = TRUE;
	defaults.isi_drawxaxis = TRUE;
	defaults.isi_drawyaxis = TRUE;
	defaults.psip_drawxaxis = TRUE;
	defaults.psip_drawyaxis = TRUE;
	defaults.ccor_drawxaxis = TRUE;
	defaults.ccor_drawyaxis = TRUE;
	defaults.rast_autoysize = FALSE;
	defaults.spectrum_drawxaxis = TRUE;
	defaults.spectrum_drawyaxis = TRUE;
	defaults.rast_dotshape = DEFAULT_RASTDOTSHAPE;
	defaults.isi_xrange = DEFAULT_ISIXRANGE;
	defaults.isi_bindur = DEFAULT_ISIBINDUR;
	defaults.psth_bindur = DEFAULT_PSTHBINDUR;
	defaults.psth_ymax = DEFAULT_PSTHYMAX;
	defaults.fftsize = DEFAULT_FFTSIZE;
	defaults.fftolap = DEFAULT_FFTOLAP;
	defaults.ccor_wsize = DEFAULT_CCORWSIZE;
	defaults.ccor_tau = DEFAULT_CCORTAU;
	defaults.ccor_taustep = DEFAULT_CCORTAUSTEP;
	defaults.method = DEFAULT_METHOD;
	defaults.scale = DEFAULT_SCALE;
	defaults.windowtype = DEFAULT_WINDOWTYPE;
	defaults.npoles = DEFAULT_NPOLES;
	defaults.colormap = CMAP_GRAY;
	defaults.xmin = 0;
	defaults.xmax = 0;
	defaults.ymin = 0;
	defaults.ymax = 0;
	defaults.frmin = 0;
	defaults.frmax = 0;
	defaults.gsmin = 0;
	defaults.gsmax = 0;
	defaults.xrangemin = 0;
	defaults.xrangemax = 0;
	defaults.xpos = 0;
	defaults.width = 500;
	defaults.zoomfactor = DEFAULT_ZOOMFACTOR;
	defaults.outputwav = TRUE;
	defaults.usealsa = TRUE;
	defaults.showsliders = TRUE;
	defaults.video_height = 240;
	defaults.sono_height = 100;
	defaults.osc_height = 100;
	defaults.label_height = 100;
	defaults.rast_height = 100;
	defaults.psth_height = 100;
	defaults.isi_height = 100;
	defaults.ccor_height = 100;
	defaults.psip_height = 100;
	defaults.spectrum_height = 100;
}

int set_default(char *key, char *value)
{
	int d;

	for (d = 0; defdata[d].key != NULL; d++)
	{
		if (!strncmp(key, defdata[d].key, strlen(defdata[d].key)))
		{
			if (!strcmp(defdata[d].type, "boolean"))
			{
				*(int *)(defdata[d].tgt) = (!strncasecmp(value, "true", 4)) ? TRUE : FALSE;
			}
			else if (!strcmp(defdata[d].type, "long"))
			{
				*(long *)(defdata[d].tgt) = atol(value);
			}
			else if (!strcmp(defdata[d].type, "float"))
			{
				*(float *)(defdata[d].tgt) = strtod(value, NULL);
			}
			else if (!strcmp(defdata[d].type, "int"))
			{
				*(int *)(defdata[d].tgt) = atoi(value);
			}
			else
				return -1;
			break;
		}
	}
	return 0;
}

void load_defaults(int defsect)
{
	char *fname, *home;
	char *ch, buf[256];
	FILE *fp;
	int d;

	if ((home = getenv("HOME")) != NULL)
	{
		fname = (char *)malloc(strlen(home) + 80);
		sprintf(fname, "%s/.aplot/defaults_%d", home, defsect);
		if ((fp = fopen(fname, "r")) != NULL)
		{
			while (fgets(buf, sizeof(buf), fp))
			{
				if (buf[0] != '#')
				{
					for (d = 0; defdata[d].key != NULL; d++)
					{
						if (!strncmp(buf, defdata[d].key, strlen(defdata[d].key)))
						{
							if ((ch = strchr(buf, '=')) != NULL)
							{
								if (!strcmp(defdata[d].type, "boolean"))
									*(int *)(defdata[d].tgt) = (!strncasecmp(ch + 1, "true", 4)) ? TRUE : FALSE;
								else if (!strcmp(defdata[d].type, "long"))
									*(long *)(defdata[d].tgt) = atol(ch + 1);
								else if (!strcmp(defdata[d].type, "float"))
									*(float *)(defdata[d].tgt) = strtod(ch + 1, NULL);
								else if (!strcmp(defdata[d].type, "int"))
									*(int *)(defdata[d].tgt) = atoi(ch + 1);
							}
						}
					}
				}
			}
		}
		free(fname);
	}
	else fprintf(stderr, "Couldn't identify your home directory! - Not loading defaults.\n");
	return;
}

void save_defaults(int defsect)
{
	char *fname, *home;
	FILE *fp;
	void *tgt;
	int d;

	if ((home = getenv("HOME")) != NULL)
	{
		fname = (char *)malloc(strlen(home) + 80);
		sprintf(fname, "%s/.aplot/defaults_%d", home, defsect);
		if ((fp = fopen(fname, "w")) != NULL)
		{
			for (d = 0; defdata[d].key != NULL; d++)
			{
				fprintf(fp, "%s[%s]=", defdata[d].key, defdata[d].type);
				tgt = defdata[d].tgt;
				if (!strcmp(defdata[d].type, "boolean"))
					fprintf(fp, "%s\n", (*(int *)tgt == 0) ? "false" : "true");
				else if (!strcmp(defdata[d].type, "int"))
					fprintf(fp, "%d\n", *(int *)tgt);
				else if (!strcmp(defdata[d].type, "long"))
					fprintf(fp, "%ld\n", *(long *)tgt);
				else if (!strcmp(defdata[d].type, "float"))
					fprintf(fp, "%f\n", *(float *)tgt);
			}
		}
		free(fname);
	}
	else fprintf(stderr, "Couldn't identify your home directory! - Not saving defaults.\n");
	return;
}

