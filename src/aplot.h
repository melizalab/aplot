#include <Xm/Xm.h>
#include <Xm/PanedW.h>
#include <X11/StringDefs.h>  
#include <X11/Intrinsic.h> 
#include <X11/Xutil.h>
#include "XCC.h"


#include "pcmio.h"
#include "lblio.h"
#include "toeio.h"
#include "vidio.h"
#include "sonogram.h"
#include "aplot_defaults.h"

#if XmVersion >= 1002
#define XmStringCreateLtoR(a, b) XmStringCreateSimple(a)
#endif

#ifdef VMS
typedef unsigned short ushort;
#endif

#ifndef __osf__
#ifdef sun
typedef unsigned long ulong;
#endif
#endif

#ifdef sun
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000
#endif
#endif

#ifndef MAXFLOAT
#ifdef FLT_MAX
#define MAXFLOAT FLT_MAX
#else
#define MAXFLOAT HUGE_VAL
#endif
#endif

typedef uint boolean;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define SUCCESS 1
#define ERROR 2

#define SND_FRAMES_PER_BUFFER   (1024)

#define MAXCOLORS 256			/* max possible colors */
#define MAX_APP_COLORS 64		/* max colors used by this app */

#define PRINT_TO_LPR    0
#define PRINT_TO_SINGLE 1
#define PRINT_TO_MANY   2
#define PRINT_TO_XFIG	3

/**********************************************************************/
 /* macros */
 /**********************************************************************/
#define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )
#define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define KEY_IS_UP(a) ( (!strcmp(a, "Up")) || (!strcmp(a, "osfUp")) )
#define KEY_IS_DOWN(a) ( (!strcmp(a, "Down")) || (!strcmp(a, "osfDown")) )
#define KEY_IS_LEFT(a) ( (!strcmp(a, "Left")) || (!strcmp(a, "osfLeft")) )
#define KEY_IS_RIGHT(a) ( (!strcmp(a, "Right")) || (!strcmp(a, "osfRight")) )
#define KEY_IS_DELETE(a) ( (!strcmp(a, "Delete")) || (!strcmp(a, "osfDelete")) )
#define KEY_IS_BACKSPACE(a) ( (!strcmp(a, "BackSpace")) || (!strcmp(a, "osfBackSpace")) )

typedef struct group GROUP;
typedef struct plot PLOT;
typedef struct panel PANEL;

struct plot
{
	PLOT *next;
	char name[132];
	int type;
	char typename[20];
	int entry;
	GROUP *group;
	PANEL *panel;

	int dirty;
	boolean created;
	Widget plot_widget;
	Widget plot_popupmenu_widget;

	int (*plot_open)();
	int (*plot_display)();
	int (*plot_set)();
	int (*plot_close)();
	int (*plot_print)();
	int (*plot_save)();
	int (*plot_play)();
	void (*plot_playmarker)(PLOT *, float);
	void (*plot_clearmarks)(PLOT *);
	void (*plot_drawstartmark)(PLOT *, float);
	void (*plot_drawstopmark)(PLOT *, float);
	void (*plot_event_handler)(Widget, XtPointer, XEvent *, Boolean *);
	void (*plot_showvideoframe)(PLOT *, unsigned long long);
	float (*plot_conv_pixel_to_time)(PLOT *, int);

	Boolean noiseclip;

	Boolean drawxaxis, drawyaxis;
	XmFontList ticklblfont;
	Dimension height, width;
	int offx,offy,offx2,offy2;
	int minoffx, minoffy, minoffx2, minoffy2;
	int depth;
	short markx1, markx2, playmark;
	void *plotdata;
};

struct group
{
	GROUP *next;
	int groupnum;
	char filename[132];
	char loadedfilename[132];
	long entry;
	int numplots;

	int dirty;
	int pcmdirty, lbldirty, toedirty, viddirty;

	PCMFILE *pcmfp;
	int needpcm, needlbl, needtoe, needvid;
	short *samples;
	int nsamples;
	unsigned long long ntime;

	LBLFILE *lblfp;
	float *lbltimes;
	char **lblnames;
	int lblcount;

	TOEFILE *toefp;
	float **toe_repptrs;
	int *toe_repcounts;
	int toe_nreps;
	int toe_nunits;
	int toe_datacount;
	int toe_curunit;

	VIDFILE *vidfp;
	void *framedata;
	int nframes;

	char filename2[132];
	char loadedfilename2[132];
	long entry2;

	TOEFILE *toe2fp;
	float **toe2_repptrs;
	int *toe2_repcounts;
	int toe2_nreps;
	int toe2_nunits;
	int toe2_datacount;
	int toe2_curunit;

	PLOT *plots;

	long fftsize;
	long fftolap;
	int samplerate;
	float start, stop;
	float xrangemin, xrangemax;
	long ymin, ymax, gsmin, gsmax, frmin, frmax;
	long xpos;
	long zoomhist_size, zoomhist_count;
	float *zoomhist_min, *zoomhist_max;
	boolean sonoxgrid, sonoygrid;
	boolean loadall;

	float psth_bindur;			/* PSTH plot: Bin duration (in ms) */
	float isi_bindur;			/* ISI plot: Bin duration (in ms) */

	GROUP *next_sel;
	char group_plottypes[132];
	char group_panellistline[256];
	int listpos;															/* Position in the selection list */
	boolean selected;
};

struct panel
{
	PANEL *next;
	char name[132];
	int num;
	GROUP *groups;
	int nextgroupnum;
	int numplots;
	int numgroups;

	boolean realized;
	Widget panel_shell;
	Widget panel_container;
	Widget panel_scale;
	Widget panel_scrollbar;
	int container_height;
	int container_width;

	PANEL *next_sel;
	GROUP *groups_sel;
	int listpos;															/* Position in the selection list */
	boolean selected;
};

extern PANEL *selpanel;

#define CMAP_GRAY   0
#define CMAP_HOT    1
#define CMAP_COOL   2
#define CMAP_BONE   3

typedef struct
{
	void (*cmap)(Display *, int, unsigned long *, unsigned long *);
} cmap_fns;
extern cmap_fns colormap[];

typedef struct
{
	Pixel bg;
	String *geometry;
	String *sono_geometry;
	String *osc_geometry;
	int grouplbl;
	String *dottype;
	int outputseq2;
	int defaults_section;
} ApplicationData, *ApplicationDataPtr;

extern Widget toplevel;
extern ApplicationData resource_data;
extern struct defaultsdata defaults;

typedef struct
{
	float x1, y1, x2, y2;
} PSSegment;

typedef struct
{
	float x, y;
} PSPoint;

typedef struct
{
	float x, y;
	float width, height;
} PSRectangle;

void flush_xevents(void);

void init_ui(int argc, char *argv[]);
char *mybasename(char *pathname);

extern void init_defaults(void);
extern int set_default(char *key, char *value);
extern void load_defaults(int defsect);
extern void save_defaults(int defsect);
extern void ui_get_gs_range(int *gsmin_p, int *gsmax_p);
extern void ui_set_gs_range(int gsmin, int gsmax);

int panel_open(PANEL *pan);
int panel_close(PANEL *pan);
int panel_addplotgroup(PANEL *pan, char *filename, long entry, char *types, GROUP **group_p, char *filename2);
int panel_deleteplotgroup(PANEL *pan, long groupnum);
int panel_display(PANEL *pan);
int panel_set(PANEL *pan, char *types, int modifiermask);
int panel_print(PANEL *pan, int printtype, char *filename);
int panel_save(PANEL *pan);
int panel_play(PANEL *pan);
void panel_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
void panel_handle_keyrelease(Widget w, PLOT *plot, XKeyEvent *keyevent, char *keybuf);

void panel_showvideoframe(PANEL *panel, unsigned long long ntime);
void panel_clearmarks(PANEL *panel);
void panel_playmarker(GROUP *group, float t);
void panel_drawstartmark(PANEL *panel, float t);
void panel_drawstopmark(PANEL *panel, float t);

void panel_panleft(PANEL *panel, float percentage);
void panel_panright(PANEL *panel, float percentage);
void panel_zoom(PANEL *panel, float tmin, float tmax);
void panel_unzoom(PANEL *panel);
void update_fields(PANEL *pan);

int load_pcmdata(PCMFILE **pcmfp_p, char *loadedfilename, char *filename, int entry, 
	short **samples_p, int *nsamples_p, int *samplerate_p, float *start_p, float *stop_p,
	unsigned long long *ntime_p);

int load_lbldata(LBLFILE **lblfp_p, char *loadedfilename, char *filename, int entry, 
	int *lblcount_p, float **lbltimes_p, char ***lblnames_p, float *start_p, float *stop_p);

int load_toedata(TOEFILE **toefp_p, char *loadedfilename, char *filename, int entry, int unit,
	int *datacount_p, int *nunits_p, int *nreps_p, int **repcounts_p, float ***repptrs_p,
	float *start_p, float *stop_p);

int load_viddata(VIDFILE **vidfp_p, char *loadedfilename, char *filename, int entry, 
	int *nframes_p, int *width_p, int *height_p, int *ncomps_p, int *microsecs_per_frame_p,
	unsigned long long *ntime_p);

void cursor_set_busy(Widget w);
void cursor_unset_busy(Widget w);

void DrawXAxis(Display *dpy, Pixmap pixmap, GC gc, XmFontList ticklblfont, int offx, int offy, int width, int height, float x0, float x1);
void DrawYAxis(Display *dpy, Pixmap pixmap, GC gc, XmFontList ticklblfont, int offx, int offy, int width, int height, float y0, float y1);
void DrawXGrid(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height, float x0, float x1);
void DrawYGrid(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height);
XFontStruct *getFontStruct(XmFontList font);


void PSInit(FILE *fp, char *filename, int width, int height);
void PSFinish(FILE *fp);
void PSDrawLine(FILE *fp, int x1, int y1, int x2, int y2);
void PSDrawLineFP(FILE *fp, float x1, float y1, float x2, float y2);
void PSDrawLines(FILE *fp, XPoint *pts, int nsegs);
void PSDrawLinesFP(FILE *fp, PSPoint *pts, int nsegs);
void PSDrawPoints(FILE *fp, XPoint *pts, int nsegs);
void PSDrawPointsFP(FILE *fp, PSPoint *pts, int nsegs);
void PSDrawSpikesFP(FILE *fp, PSPoint *pts, int nsegs);
void PSDrawFBoxFP(FILE *fp, float width, float height, float bottom, float left);
void PSFillRectangles(FILE *fp, PSRectangle *rects, int nrects);
void PSDrawSegments(FILE *fp, XSegment *segs, int nsegs);
void PSDrawSegmentsFP(FILE *fp, PSSegment *segs, int nsegs);
void PSDrawXAxisLabel(FILE *fp, char *string, int x, int y, int width);
void PSDrawYAxisLabel(FILE *fp, char *string, int x, int y);
void PSDrawXAxis(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height, float x0, float x1);
void PSDrawYAxis(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height, float y0, float y1);
void PSDrawXGrid(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height, float x0, float x1);
void PSDrawYGrid(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height);
void PSInit_parent(FILE *fp);
void PSIncludeEPSF(FILE *fp, char *filename, int xpos, int ypos, int width, int height);
void PSFinishEPSF(FILE *fp);
void PSDrawHorizText(FILE *fp, char *string, float x, float y);
void PSSet_Pref(char *property, char *value);
void PSDrawFBoxesFP(FILE *fp, PSPoint *pts, int nsegs, float width, float y);

int ccor_open(PLOT *pdata);
int ccor_display(PLOT *pdata);
int ccor_set(PLOT *pdata);
int ccor_close(PLOT *pdata);
int ccor_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int ccor_save(PLOT *pdata);

int isi_open(PLOT *pdata);
int isi_display(PLOT *pdata);
int isi_set(PLOT *pdata);
int isi_close(PLOT *pdata);
int isi_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int isi_save(PLOT *pdata);

int label_open(PLOT *pdata);
int label_display(PLOT *pdata);
int label_set(PLOT *pdata);
int label_close(PLOT *pdata);
int label_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int label_save(PLOT *pdata);

int osc_open(PLOT *pdata);
int osc_display(PLOT *pdata);
int osc_set(PLOT *pdata);
int osc_close(PLOT *pdata);
int osc_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int osc_save(PLOT *pdata);

int psth_open(PLOT *pdata);
int psth_display(PLOT *pdata);
int psth_set(PLOT *pdata);
int psth_close(PLOT *pdata);
int psth_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int psth_save(PLOT *pdata);

int rast_open(PLOT *pdata);
int rast_display(PLOT *pdata);
int rast_set(PLOT *pdata);
int rast_close(PLOT *pdata);
int rast_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int rast_save(PLOT *pdata);

int psip_open(PLOT *pdata);
int psip_display(PLOT *pdata);
int psip_set(PLOT *pdata);
int psip_close(PLOT *pdata);
int psip_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int psip_save(PLOT *pdata);

int sono_open(PLOT *pdata);
int sono_display(PLOT *pdata);
int sono_set(PLOT *pdata);
int sono_close(PLOT *pdata);
int sono_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int sono_save(PLOT *pdata);

int spectrum_open(PLOT *pdata);
int spectrum_display(PLOT *pdata);
int spectrum_set(PLOT *pdata);
int spectrum_close(PLOT *pdata);
int spectrum_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int spectrum_save(PLOT *pdata);

int video_open(PLOT *pdata);
int video_display(PLOT *pdata);
int video_set(PLOT *pdata);
int video_close(PLOT *pdata);
int video_print(PLOT *pdata, FILE *fp, char **ret_filename_p);
int video_save(PLOT *pdata);

void report_size(PLOT *, char *);

int init_sound(int dacrate, int ndacchans);
void close_sound(void);
int write_sound(short *databuf, int cnt);


