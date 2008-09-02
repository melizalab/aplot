
/**********************************************************************************
 * INCLUDES
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#ifndef VMS
#include <unistd.h>
#endif
#include <Xm/DrawingA.h>
#include <Xm/RowColumn.h>
#include <Xm/MenuShell.h>
#include <Xm/LabelG.h>
#include <Xm/SeparatoG.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/CascadeB.h>
#include <X11/cursorfont.h>
#include "aplot.h"
#include <Xm/XmP.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>


/**********************************************************************************
 * STRUCTURES
 **********************************************************************************/
struct psipdata
{
	GC drawing_GC, inverse_GC, mark_GC;
	Pixmap pixmap;
	Boolean pixmapalloced;
	Boolean butgrabbed;
	XmFontList lblfont;
	int xmin,xmax,ymin,ymax;
	float *intervals;
	float nintervals;
	int smoothingpoints;
};


/**********************************************************************************
 * DEFINES
 **********************************************************************************/
/*
** checking for something to be INBOUNDS means checking if a pixel in the window falls
** within the boundaries of the plot (as opposed to being in the axis area, for example.)
*/
#define IS_X_INBOUNDS(plot, x) ((x >= plot->offx) && (x <= (plot->offx + plot->width)))
#define IS_Y_INBOUNDS(plot, y) ((y >= plot->offy) && (y <= (plot->offy + plot->height)))
#define CONV_VALUE_TO_Y(ymin, ymax, height, offy, value) (offy + height - (((value - ymin) / (ymax - ymin)) * (height)))
#define CONV_MS_TO_X(plot, t) \
	(((((t) - plot->group->xrangemin) / (plot->group->xrangemax - plot->group->xrangemin)) * (plot->width)) + plot->offx)

/*
** DRAW_MARK draws a dashed vertical line, and is used when a range is being selected.
*/
#define DRAW_MARK(plot, plotdata, x) \
		{ \
			XDrawLine(XtDisplay(plot->plot_widget), XtWindow(plot->plot_widget), plotdata->mark_GC, \
				x, plot->offy, \
				x, plot->offy + plot->height); \
		}

#define CreateMenuToggle(widget, parent, name, label, set, userdata) \
	widget = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNset, (set), 															\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNvalueChangedCallback, psip_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);

#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, psip_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int psip_open(PLOT *plot);
int psip_display(PLOT *plot);
int psip_set(PLOT *plot);
int psip_close(PLOT *plot);
int psip_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int psip_save(PLOT *plot);
static void psip_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void psip_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void psip_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void psip_clearmarks(PLOT *plot);
static void psip_drawstartmark(PLOT *plot, float t);
static void psip_drawstopmark(PLOT *plot, float t);
static void calc_statistics(PLOT *plot, float tmin, float tmax);
static void psip_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);
static int intervalcomp(const void *A, const void *B);
static void DrawPSIP(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float *intervals, int nintervals, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, int ymin, int ymax);
static void PSDrawPSIP(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float *intervals, int nintervals, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, int ymin, int ymax);

float psip_conv_pixel_to_time(PLOT *plot, int x);
float psip_conv_pixel_to_val(PLOT *plot, int y);


/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int psip_open(PLOT *plot)
{
	struct psipdata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3, m4, m5, m6, m7;
	Widget m6a, m6a1, m6a2, m6a3, m6a4, m6a5, m6a6, m6a7, m6a8;
	Widget m7a, m7a1, m7a2, m7a3, m7a4, m7a5, m7a6, m7a7, m7a8;
	int status = SUCCESS;
	Dimension width, height;
	int depth, offx, offy, offx2, offy2;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct psipdata));
	plotdata = (struct psipdata *)plot->plotdata;

	plotdata->intervals = NULL;
	plotdata->nintervals = 0;
	plotdata->smoothingpoints = 10;
	plotdata->ymin = 0;
	plotdata->ymax = 50;

	plot->plot_widget = XtVaCreateManagedWidget("psip", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.psip_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);
	XtAddCallback(plot->plot_widget, XmNexposeCallback, psip_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, psip_resize_callback, (XtPointer)plot);

	w = (Widget)plot->plot_widget;
	dpy = XtDisplay(w);
	scr = XtScreen(w);

	XtVaGetValues(w,
		XmNheight, &height,
		XmNwidth, &width,
		XmNdepth, &depth,
		XmNforeground, &foreground,
		XmNbackground, &background,
		NULL);

	/*
	** Get the font;  also, calculate the margins for the axes (this depends on the font size!).
	** Store these margins for use later (we might start off without axes, and turn them on later.)
	*/
	plot->ticklblfont = XmFontListCopy(_XmGetDefaultFontList(w, XmLABEL_FONTLIST));
	plot->minoffx = 6 + XmStringWidth(plot->ticklblfont, xmstr = XmStringCreateSimple("-32768")); XmStringFree(xmstr);
	plot->minoffx2 = 0;
	plot->minoffy = 0;
	plot->minoffy2 = 6 + XmStringHeight(plot->ticklblfont, xmstr = XmStringCreateSimple("1")); XmStringFree(xmstr);

	plot->drawxaxis = defaults.psip_drawxaxis;
	plot->drawyaxis = defaults.psip_drawyaxis;

	offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
	offy = plot->offy = 0;
	offx2 = plot->offx2 = 0;
	offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
	plot->width = width - offx - offx2;
	plot->height = height - offy - offy2;

	plot->depth = depth;

	/*
	** Create the Graphics contexts.
	** drawing_GC is for the psip itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
	*/
	values.font = getFontStruct(plot->ticklblfont)->fid;
	values.foreground = foreground;
	values.background = background;
	plotdata->drawing_GC = XtGetGC(w, GCForeground | GCBackground | GCFont, &values);
	values.foreground = background;
	values.background = background;
	plotdata->inverse_GC = XtGetGC(w, GCForeground | GCBackground, &values);
	values.function = GXxor;
	values.plane_mask = foreground ^ background;
	values.foreground = 0xffffffff;
	values.background = 0x0;
	values.line_style = LineSolid;
	plotdata->mark_GC = XtGetGC(w, GCFunction | GCPlaneMask | GCForeground | GCBackground | GCLineStyle, &values);

	plotdata->pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), width + offx + offx2, height + offy + offy2, depth);
	plotdata->pixmapalloced = TRUE;
	XFillRectangle(dpy, plotdata->pixmap, plotdata->inverse_GC, 0, 0, width + offx + offx2, height + offy + offy2);

	/*
	** Create the popup menu
	** 
	*/
	n = 0;
	XtSetArg(args[n], XmNmenuPost, "<Btn3Down>"); n++;
	mw = plot->plot_popupmenu_widget = XmCreatePopupMenu(w, "popupmenu", args, n);

	m0 = XtVaCreateManagedWidget("m0", xmLabelGadgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Options"),
		NULL);
	XmStringFree(xmstr1);

	m1 = XtVaCreateManagedWidget("m1", xmSeparatorGadgetClass, mw,
		NULL);

	CreateMenuToggle(m2, mw, "xaxis", "X axis", plot->drawxaxis, "X axis");
	CreateMenuToggle(m3, mw, "yaxis", "Y axis", plot->drawyaxis, "Y axis");
	CreateMenuButton(m4, mw, "save", "Save", "Save");
	CreateMenuButton(m5, mw, "print", "Print EPS", "Print EPS");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m6a = XmCreatePulldownMenu(mw, "m6a", args, n);
	m6 = XtVaCreateManagedWidget("unit", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Unit"),
		XmNsubMenuId, m6a,
		NULL);
	CreateMenuToggle(m6a1, m6a, "unit1", "1", (plot->group->toe_curunit == 1), "Unit 1");
	CreateMenuToggle(m6a2, m6a, "unit2", "2", (plot->group->toe_curunit == 2), "Unit 2");
	CreateMenuToggle(m6a3, m6a, "unit3", "3", (plot->group->toe_curunit == 3), "Unit 3");
	CreateMenuToggle(m6a4, m6a, "unit4", "4", (plot->group->toe_curunit == 4), "Unit 4");
	CreateMenuToggle(m6a5, m6a, "unit5", "5", (plot->group->toe_curunit == 5), "Unit 5");
	CreateMenuToggle(m6a6, m6a, "unit6", "6", (plot->group->toe_curunit == 6), "Unit 6");
	CreateMenuToggle(m6a7, m6a, "unit7", "7", (plot->group->toe_curunit == 7), "Unit 7");
	CreateMenuToggle(m6a8, m6a, "unit8", "8", (plot->group->toe_curunit == 8), "Unit 8");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m7a = XmCreatePulldownMenu(mw, "m7a", args, n);
	m7 = XtVaCreateManagedWidget("smooth", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Smooth points"),
		XmNsubMenuId, m7a,
		NULL);
	CreateMenuToggle(m7a1, m7a, "smooth1", "1", (plotdata->smoothingpoints == 1), "Smooth 1");
	CreateMenuToggle(m7a2, m7a, "smooth2", "3", (plotdata->smoothingpoints == 3), "Smooth 3");
	CreateMenuToggle(m7a3, m7a, "smooth3", "5", (plotdata->smoothingpoints == 5), "Smooth 5");
	CreateMenuToggle(m7a4, m7a, "smooth4", "7", (plotdata->smoothingpoints == 7), "Smooth 7");
	CreateMenuToggle(m7a5, m7a, "smooth5", "10", (plotdata->smoothingpoints == 10), "Smooth 10");
	CreateMenuToggle(m7a6, m7a, "smooth6", "20", (plotdata->smoothingpoints == 20), "Smooth 20");
	CreateMenuToggle(m7a7, m7a, "smooth7", "30", (plotdata->smoothingpoints == 30), "Smooth 30");
	CreateMenuToggle(m7a8, m7a, "smooth8", "50", (plotdata->smoothingpoints == 50), "Smooth 50");

	/*
	** Register an event handler
	*/
	XtAddEventHandler(w, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
		FALSE, (XtEventHandler) psip_event_handler, (XtPointer)plot);

	plot->plot_clearmarks = psip_clearmarks;
	plot->plot_drawstartmark = psip_drawstartmark;
	plot->plot_drawstopmark = psip_drawstopmark;
	plot->plot_conv_pixel_to_time = psip_conv_pixel_to_time;

	plot->plot_display = psip_display;
	plot->plot_set = psip_set;
	plot->plot_close = psip_close;
	plot->plot_print = psip_print;
	plot->plot_save = psip_save;

	plot->plot_event_handler = psip_event_handler;

	plotdata->butgrabbed = FALSE;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needtoe = 1;

	return status;
}

int psip_display(PLOT *plot)
{
	int status = SUCCESS, tstatus;
	GROUP *group = plot->group;

	/*
	** Load the toe data, but only if we need to...
	*/
	if (group->toedirty == 1)
	{
		group->samplerate = 20000;
		if (load_toedata(&group->toefp, group->loadedfilename, group->filename, group->entry, group->toe_curunit,
			&group->toe_datacount, &group->toe_nunits, &group->toe_nreps,
			&group->toe_repcounts, &group->toe_repptrs, &group->start, &group->stop) == 0)
		{
			group->toedirty = 0;
			group->xrangemin = group->start;
			group->xrangemax = group->stop;
		}
		else status = ERROR;
	}

	tstatus = psip_set(plot);
	return (status == SUCCESS) ? (tstatus) : (status);
}

static int intervalcomp(const void *A, const void *B)
{
	float t1, t2;

	t1 = *(float *)A;
	t2 = *(float *)B;
	return (t1 < t2) ? (-1) : ( (t1 > t2) ? (1) : (0) );
}

/*
** Before calling this function,
** you can:
** 		resize the window,				modify XmNwidth, XmNheight of widget
**		zoom/unzoom						modify plot->group->start,xrangemin,xrangemax
**		change the y-range				modify plot->group->ymin,ymax
**		turn x/y axes on/off			modify plot->drawxaxis,drawyaxis
**		change the axis font			modify plot->ticklblfont,minoffx,minoffy,minoffx2,minoffy2
*/
int psip_set(PLOT *plot)
{
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	int xmin, xmax, ymin, ymax, offx, offy, offx2, offy2;
	float **repptrs;
	int *repcounts, nreps;
	int i, j, k, N, count, nintervals;
	float miny, maxy, *tmpints, *intervals, *data;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to psip_open().
	*/
	if (plotdata->butgrabbed == FALSE)
	{
		XGrabButton(dpy, AnyButton, AnyModifier, XtWindow(w), TRUE,
			ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
			GrabModeAsync, GrabModeAsync, XtWindow(w),
			XCreateFontCursor(dpy, XC_crosshair));
		plotdata->butgrabbed = TRUE;
	}

	if (plotdata->pixmapalloced)
	{
		/*
		** Retrieve the window size
		*/
		XtVaGetValues(w,
			XmNheight, &height,
			XmNwidth, &width,
			NULL);
		defaults.psip_height = height;
		defaults.width = width;

		/*
		** Set the axes margins (note: if you want to change
		** the ticklblfont, then you must also update the
		** plot->minoff?? numbers.  They are not recalculated
		** here.)  (see psip_open)
		*/
		offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
		offy = plot->offy = 0;
		offx2 = plot->offx2 = 0;
		offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
		plot->width = width - offx - offx2;
		plot->height = height - offy - offy2;
		width = plot->width;
		height = plot->height;

		/*
		** If the window size has changed, we need to reallocate the
		** pixmap.
		*/
		XGetGeometry(dpy, plotdata->pixmap, &root_return, &x_return, &y_return,
			&pixwidth, &pixheight, &border_width_return, &depth_return);
		if ((pixheight != height + offy + offy2) || (pixwidth != width + offx + offx2))
		{
			XFreePixmap(dpy, plotdata->pixmap);
			plotdata->pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy),
				width + offx + offx2, height + offy + offy2, plot->depth);
		}

		XFillRectangle(dpy, plotdata->pixmap, plotdata->inverse_GC, 0, 0, width + offx + offx2, height + offy + offy2);

		/*
		** Calculate the bounds: the first and last sample value to plot, and the y-axis bounds.
		** Fix up the bounds if necessary.
		*/
		xmin = plot->group->xrangemin * (plot->group->samplerate / 1000.0);
		xmax = plot->group->xrangemax * (plot->group->samplerate / 1000.0);

		nreps = plot->group->toe_nreps;
		repptrs = plot->group->toe_repptrs;
		repcounts = plot->group->toe_repcounts;

		ymin = plotdata->ymin;
		ymax = plotdata->ymax;
		intervals = plotdata->intervals;
		nintervals = plotdata->nintervals;

		if ((plot->group->dirty == 1) || 
			(plot->dirty == 1) ||
			(plot->group->ymin != ymin) ||
			(plot->group->ymax != ymax) ||
			(plotdata->xmin != xmin) ||
			(plotdata->xmax != xmax))
		{
			plot->dirty = 0;
			if (plotdata->intervals != NULL) free(plotdata->intervals);
			plotdata->intervals = NULL;
			plotdata->nintervals = 0;
			if ((intervals = (float *)calloc(2 * plot->group->toe_datacount, sizeof(float))) != NULL)
			{
				/*
				** Store, as a pair of floats, the time of each spike, along with the
				** interval preceding it. (Skip the first spike of each rep)
				*/
				for (k=0, i=1; i <= nreps; i++)
				{
					data = repptrs[i];
					count = repcounts[i];
					while ((count >= 0) && (*data < plot->group->xrangemin)) { data++; count--; };
					while ((count >= 0) && (data[count-1] >= plot->group->xrangemax)) { count--; };
					for (j=1; j < count; j++)
					{
						intervals[k++] = data[j];
						intervals[k++] = data[j] - data[j-1];
					}
				}
				k = k / 2;

				/*
				** Sort the intervals by spike time relative to stimulus onset.
				** Remember that intervals is an array of paired floats.
				** Perhaps heapsort would be better for this...
				*/
				qsort(intervals, k, 2 * sizeof(float), intervalcomp);

				/*
				** Smooth the intervals, using a running N-point mean 
				*/
				N = plotdata->smoothingpoints;
				if (N > 1)
				{
					float tot;
					int num;

					tmpints = (float *)calloc(plot->group->toe_datacount, sizeof(float));

					for (i=0; i < k; i++)
					{
						int l;
						for (l = 0, tot = 0.0, num = 0, j = MAX(0, i - (N/2)); l < N; j++, l++)
						{
							if (j < k)
							{
								tot += intervals[2*j + 1];
								num++;
							}
						}
						tot /= num;
						tmpints[i] = tot;
					}
					for (i=0; i < k; i++)
						intervals[2*i + 1] = tmpints[i];

					free(tmpints);
				}

				/*
				** Compute instantaneous frequencies (the reciprocals of each interval)
				** Plotting these makes the plots more comparable to PSTH's...
				*/
				maxy = 0.0;
				miny = MAXFLOAT;
				for (i=0; i < k; i++)
				{
					if (intervals[2*i + 1] > -1 * 0.000000001)
					{
						intervals[2*i + 1] = 1000.0 / intervals[2*i + 1];
						if (intervals[2*i + 1] > maxy)
							maxy = intervals[2*i + 1];
						if (intervals[2*i + 1] < miny)
							miny = intervals[2*i + 1];
					}
					else
						intervals[2*i + 1] = MAXFLOAT;
				}

				/*
				** Store the results in plot
				*/
				plotdata->intervals = intervals;
				plotdata->nintervals = k;
				nintervals = k;

				if (plot->group->ymin == plot->group->ymax)
				{
					fprintf(stderr, "min/max = %f, %f\n", miny, maxy);
					ymax = 10 * (0 + (int)ceil(maxy / 10.0));
					ymin = 10 * (0 + (int)floor(miny / 10.0));
					fprintf(stderr, "min/max y = %d, %d\n", ymin, ymax);
				}
				else
				{
					ymin = plot->group->ymin;
					ymax = plot->group->ymax;
				}
			}
			else fprintf(stderr, "WARNING: Memory allocation failed in psip_set()\n");
		}

		plotdata->xmin = xmin;
		plotdata->xmax = xmax;
		plotdata->ymin = ymin;
		plotdata->ymax = ymax;

		/*
		** Clear any marks
		*/
		plot->markx1 = -1;
		plot->markx2 = -1;

		/*
		** Do the rendering to an off-screen pixmap.
		*/
		DrawPSIP(dpy, plotdata->pixmap, plotdata->drawing_GC, offx, offy, width, height,
			intervals, nintervals, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			xmin, xmax, ymin, ymax);

		if (plot->drawxaxis == TRUE)
			DrawXAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
				offx, offy, width, height, plot->group->xrangemin, plot->group->xrangemax);

		if (plot->drawyaxis == TRUE)
			DrawYAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
				offx, offy, width, height, ymin, ymax);

		XCopyArea(dpy, plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
			0, 0, width + offx + offx2, height + offy + offy2, 0, 0);
	}
	return SUCCESS;
}

int psip_close(PLOT *plot)
{
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;
	Widget w = (Widget)plot->plot_widget;

	if (plot->ticklblfont != NULL)
		XmFontListFree(plot->ticklblfont);
	if (plotdata->pixmapalloced)
	{
		XFreePixmap(XtDisplay(w), plotdata->pixmap);
		plotdata->pixmapalloced = FALSE;
	}
	XtReleaseGC(w, plotdata->drawing_GC);
	XtReleaseGC(w, plotdata->inverse_GC);
	XtReleaseGC(w, plotdata->mark_GC);
	XtDestroyWidget(plot->plot_widget);
	free(plot->plotdata);
	plot->plotdata = NULL;
	return SUCCESS;
}

int psip_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;
	Dimension width, height;
	char filenamestore[256], *filename, *ch, tempstr[40];
	int xmin, xmax, ymin, ymax, offx, offy, offx2, offy2;
	float **repptrs;
	int *repcounts, nreps;
	int fileopenflag = 0;

	/*
	** Create the filename of the output file:
	** 1) start with the input filename
	** 2) remove the extension
	** 3) add the extension: _psip_<entry>_<startms>_<stopms>.eps
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->filename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_psip_%ld_%d_%d.eps", plot->group->entry, (int)plot->group->xrangemin, (int)plot->group->xrangemax);
	strcat(filename, tempstr);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '/')) ch--;
	if (*ch == '/') filename = ++ch;

	/*
	** Retrieve the window size
	*/
	XtVaGetValues(plot->plot_widget,
		XmNheight, &height,
		XmNwidth, &width,
		NULL);

	if (fp == NULL)
	{
		fp = fopen(filename, "w");
		fileopenflag = 1;
	}

	if (fp != NULL)
	{
		PSInit(fp, filename, width, height);

		/*
		** Do the rendering to an off-screen pixmap.
		*/
		offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
		offy = plot->offy = 0;
		offx2 = plot->offx2 = 0;
		offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
		height = plot->height;
		width = plot->width;
		xmin = plot->group->xrangemin * (plot->group->samplerate / 1000.0);
		xmax = plot->group->xrangemax * (plot->group->samplerate / 1000.0);
		ymin = plotdata->ymin;
		ymax = plotdata->ymax;
		nreps = plot->group->toe_nreps;
		repptrs = plot->group->toe_repptrs;
		repcounts = plot->group->toe_repcounts;

		PSDrawPSIP(fp, offx, offy, offx2, offy2, width, height,
			plotdata->intervals, plotdata->nintervals, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			xmin, xmax, ymin, ymax);
		if (plot->drawxaxis == TRUE)
			PSDrawXAxis(fp, offx, offy, offx2, offy2, width, height, plot->group->xrangemin, plot->group->xrangemax);
		if (plot->drawyaxis == TRUE)
			PSDrawYAxis(fp, offx, offy, offx2, offy2, width, height, ymin, ymax);

		PSFinish(fp);
		if (fileopenflag == 1)
			fclose(fp);
	}
	if (ret_filename_p) *ret_filename_p = strdup(filename);
	return SUCCESS;
}

int psip_save(PLOT *plot)
{
	return SUCCESS;
}


static void psip_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;
	XExposeEvent *expevent;
	int x, y0,y1;

	/*
	** Take care of the actual psip.  Then,
	** we handle the marks.
	*/
	if (cbs != NULL)
	{
		expevent = (XExposeEvent *)cbs->event;
		if (plotdata->pixmapalloced)
		{
			XCopyArea(XtDisplay(w), plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
				expevent->x, expevent->y, expevent->width, expevent->height, expevent->x, expevent->y);
		}
		if ((x = plot->markx1) != -1)
		{
			if ((x >= expevent->x) && (x <= (expevent->x + expevent->width)))
			{
				y0 = MAX(expevent->y, plot->offy);
				y1 = MIN(expevent->y + expevent->height, plot->offy + plot->height);
				XDrawLine(XtDisplay(w), XtWindow(w), plotdata->mark_GC, x, y0, x, y1);
			}
		}
		if ((x = plot->markx2) != -1)
		{
			if ((x >= expevent->x) && (x <= (expevent->x + expevent->width)))
			{
				y0 = MAX(expevent->y, plot->offy);
				y1 = MIN(expevent->y + expevent->height, plot->offy + plot->height);
				XDrawLine(XtDisplay(w), XtWindow(w), plotdata->mark_GC, x, y0, x, y1);
			}
		}
	}
}


/*
** This will wipe out any marks
*/
static void psip_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	psip_set(plot);
	return;
}

static void psip_clearmarks(PLOT *plot)
{
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;

	if (plot->markx1 != -1)
	{
		DRAW_MARK(plot, plotdata, plot->markx1);
		plot->markx1 = -1;
	}
	if (plot->markx2 != -1)
	{
		DRAW_MARK(plot, plotdata, plot->markx2);
		plot->markx2 = -1;
	}
}

static void psip_drawstartmark(PLOT *plot, float t)
{
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;
	int x;

	if (plot->group->xrangemin >= plot->group->xrangemax)
		return;

	x = CONV_MS_TO_X(plot, t);
	if (IS_X_INBOUNDS(plot, x))
	{
		DRAW_MARK(plot, plotdata, x);
		plot->markx1 = x;
	}
}

static void psip_drawstopmark(PLOT *plot, float t)
{
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;
	int x;

	if (plot->group->xrangemin >= plot->group->xrangemax)
		return;

	x = CONV_MS_TO_X(plot, t);
	if (x != plot->markx2)								/* If there's no change, then do nothing */
	{
		if (IS_X_INBOUNDS(plot, x))
		{
			if (plot->markx2 != -1)
				DRAW_MARK(plot, plotdata, plot->markx2);	/* Erase the old one */
			DRAW_MARK(plot, plotdata, x);					/* Draw the new one */
			plot->markx2 = x;
		}
	}
}


static void calc_statistics(PLOT *plot, float tmin, float tmax)
{
	float **repptrs, *data, spkdeltas, mindelta, meantime, mean1sttime, meancnt, mean1stcnt, variance, variance1st;
	int *repcounts, nreps, nspkdeltas, spkcount, count, start, stop, rep, i;

	nreps = plot->group->toe_nreps;
	repptrs = plot->group->toe_repptrs;
	repcounts = plot->group->toe_repcounts;
	if ((repptrs == NULL) || (repcounts == NULL) || (nreps == 0))
		return;

	/*
	** Now calculate the average # of spikes within the range,
	** the average and minimum ISI,
	** and the mean occurence time of all spikes (meantime)
	** and of the first spike (mean1sttime)
	*/
	spkdeltas = 0.0;
	nspkdeltas = 0;
	spkcount = 0;
	mindelta = 1.0e34;
	meantime = 0.0;
	mean1sttime = 0.0;
	meancnt = 0;
	mean1stcnt = 0;
	for (rep=1; rep <= nreps; rep++)
	{
		data = repptrs[rep];
		count = repcounts[rep];
		start = 0;
		while ((start < count) && (data[start] < tmin)) start++;
		stop = start;
		while ((stop < count) && (data[stop] < tmax)) stop++;
		count = stop - start;
		spkcount += count;
		for (i=start+1; i < stop; i++)
		{
			if ((data[i] - data[i-1]) < mindelta)
				mindelta = data[i] - data[i-1];
			spkdeltas += data[i] - data[i-1];
			nspkdeltas++;
		}
		if (start != stop)
		{
			mean1sttime += data[start];
			mean1stcnt++;
		}
		for (i=start; i < stop; i++)
		{
			meantime += data[i];
			meancnt++;
		}
	}

	printf("avg_#spikes/rep: %f\tavg_spike_rate(Hz): %f\n", ((float)spkcount) / nreps, (float)spkcount / (nreps * ((tmax - tmin) / 1000.0)));
	if ((nspkdeltas != 0) || (spkdeltas >= 0.001))
	{
		printf("spks/rep: %.1f \tavg ISI(ms)/rate(Hz): %.2f / %.2f \tmin ISI: %.2f\n",
			((float)spkcount) / nreps, spkdeltas / nspkdeltas, (nspkdeltas * 1000.0 / spkdeltas), mindelta);
	}

	/*
	** Now calculate the variance in time of all spikes (variance)
	** and of the first spike (variance1st)
	*/
	if ((mean1stcnt > 5) && (meancnt > 5))
	{
		mean1sttime = mean1sttime / mean1stcnt;
		meantime = meantime / meancnt;
		variance = 0.0;
		variance1st = 0.0;
		for (rep=1; rep <= nreps; rep++)
		{
			data = repptrs[rep];
			count = repcounts[rep];
			start = 0;
			while ((start < count) && (data[start] < tmin)) start++;
			stop = start;
			while ((stop < count) && (data[stop] < tmax)) stop++;
			if (start != stop)
				variance1st += (data[start] - mean1sttime) * (data[start] - mean1sttime);
			for (i=start; i < stop; i++)
				variance += (data[start] - meantime) * (data[start] - meantime);
		}
		variance1st = variance1st / (mean1stcnt - 1);
		variance = variance / (meancnt - 1);
		printf("mean/std: %.2f / %.2f \t (1st spike) %.2f / %.2f\n", meantime, sqrt(variance), mean1sttime, sqrt(variance1st));
	}
}


/*
**  Key press: ctrl-x, ctrl-y, etc.
**  Button1 press: evaluate at curpos
**  Button2 press: clear marks.  start mark at curpos.
**  Button2 motion: clear old stop mark.  stop mark at curpos
**  Button2 release: same as motion.
**  Button3 press: open the popup menu
*/
static void psip_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
{
	PLOT *plot = (PLOT *)clientData;
	XButtonEvent *butevent;
	XMotionEvent *motevent;
	XKeyEvent *keyevent;
	int x;
	char *keybuf;
	KeySym key_sym;
	Modifiers mod_return;
	Widget ew;
	float tmin, tmax, ttmp;

	switch (event->type)
	{
		case KeyRelease:
			keyevent = (XKeyEvent *)event;
			ew = XtWindowToWidget(XtDisplay(w), keyevent->window);
			if (ew != plot->plot_widget)
				panel_event_handler(ew, (XtPointer)plot->panel, event, b);
			else
			{
				XtTranslateKeycode(XtDisplay(w), keyevent->keycode, 0, &mod_return, &key_sym);
				if ((keybuf = XKeysymToString(key_sym)) != NULL)
				{
					if ((keyevent->state == ControlMask) && (keybuf[0] == 'x'))
					{
						plot->drawxaxis = !(plot->drawxaxis);
						defaults.psip_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "xaxis"), XmNset, plot->drawxaxis, NULL);
						psip_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'y'))
					{
						plot->drawyaxis = !(plot->drawyaxis);
						defaults.psip_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "yaxis"), XmNset, plot->drawyaxis, NULL);
						psip_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 's'))
					{
						if ((plot->markx1 != -1) && (plot->markx2 != -1))
						{
							tmin = psip_conv_pixel_to_time(plot, plot->markx1);
							tmax = psip_conv_pixel_to_time(plot, plot->markx2);
							if (tmin > tmax)
							{
								ttmp = tmax;
								tmax = tmin;
								tmin = ttmp;
							}
							calc_statistics(plot, tmin, tmax);
						}
						else
							calc_statistics(plot, plot->group->xrangemin, plot->group->xrangemax);
					}
					else
					{
						panel_handle_keyrelease(w, plot, keyevent, keybuf);
					}
				}
			}
			break;

		case ButtonPress:
			butevent = (XButtonEvent *)event;
			switch (butevent->button)
			{
				case Button1:
					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", psip_conv_pixel_to_time(plot, butevent->x));
						if (IS_Y_INBOUNDS(plot, butevent->y))
							sprintf(title + strlen(title), " (%.0f)", psip_conv_pixel_to_val(plot, butevent->y));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
					break;

				case Button2:
					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					/* Now draw and set the start mark */
					panel_drawstartmark(plot->panel, psip_conv_pixel_to_time(plot, butevent->x));

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", psip_conv_pixel_to_time(plot, butevent->x));
						sprintf(title + strlen(title), " : %.2f (0)", psip_conv_pixel_to_time(plot, butevent->x));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
					break;

				case Button3:
					XmMenuPosition(plot->plot_popupmenu_widget, butevent);
					XtManageChild(plot->plot_popupmenu_widget);
					break;

				default:
					break;
			}
			break;

		case ButtonRelease:
			butevent = (XButtonEvent *)event;
			switch (butevent->button)
			{
				case Button1:
					XtVaSetValues(plot->panel->panel_shell, XmNtitle, plot->panel->name, NULL);
					break;
				case Button2:
					XtVaSetValues(plot->panel->panel_shell, XmNtitle, plot->panel->name, NULL);
					x = MAX(butevent->x, plot->offx);
					x = MIN(x, plot->offx + plot->width);

					/* Erase the old stop mark, and draw the new one */
					panel_drawstopmark(plot->panel, psip_conv_pixel_to_time(plot, x));

					/* If the two marks are on top of each other, they're already erased.  Clear both marks */
					if (butevent->x == plot->markx1)
					{
						plot->markx1 = -1;
						plot->markx2 = -1;
					}
					break;
				case Button3:
					break;
				default:
					break;
			}
			break;

		case MotionNotify:
			motevent = (XMotionEvent *)event;
			if (plot->markx1 != -1)
			{
				x = MAX(motevent->x, plot->offx);
				x = MIN(x, plot->offx + plot->width);

				/* Erase the old stop mark, and draw the new one */
				panel_drawstopmark(plot->panel, psip_conv_pixel_to_time(plot, x));

				if (IS_X_INBOUNDS(plot, motevent->x))
				{
					char title[256];
					float t1, t2;
					t1 = psip_conv_pixel_to_time(plot, MIN(plot->markx1, motevent->x));
					t2 = psip_conv_pixel_to_time(plot, MAX(plot->markx1, motevent->x));
					sprintf(title, "%.2f : %.2f (%.2f)", t1, t2, (t2 - t1));
					XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
				}
			}
			break;

		default:
			break;
	}
}

static void psip_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;
	char *idstring;
	Boolean drawxaxis, drawyaxis, value;
	int unit, npoints;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "X axis"))
	{
		XtVaGetValues(w, XmNset, &drawxaxis, NULL);
		plot->drawxaxis = drawxaxis;
		defaults.psip_drawyaxis = plot->drawyaxis;
		psip_set(plot);
	}
	else if (!strcmp(idstring, "Y axis"))
	{
		XtVaGetValues(w, XmNset, &drawyaxis, NULL);
		plot->drawyaxis = drawyaxis;
		defaults.psip_drawyaxis = plot->drawyaxis;
		psip_set(plot);
	}
	else if (!strncmp(idstring, "Smooth", 6))
	{
		sscanf(idstring, "Smooth %d", &npoints);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->smoothingpoints != npoints))
		{
			plot->dirty = 1;
			plotdata->smoothingpoints = npoints;
			psip_set(plot);
		}
	}
	else if (!strncmp(idstring, "Unit", 4))
	{
		plot->group->toedirty = 1;
		plot->dirty = 1;
		sscanf(idstring, "Unit %d", &unit);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plot->group->toe_curunit != unit))
		{
			plot->group->toe_curunit = unit;
			psip_display(plot);
		}
	}
	else if (!strcmp(idstring, "Save"))
	{
		psip_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		psip_print(plot, NULL, NULL);
	}
}

/*
** For the following, I employ the following notation:
** X, Y refer to pixel coordinates.
** TIME, VAL are in milliseconds and y units, respectively.
** SAMPLE is time in samples
*/
float psip_conv_pixel_to_time(PLOT *plot, int x)
{
	fprintf(stderr, "xrange: %f to %f\n", plot->group->xrangemin, plot->group->xrangemax);
	return ((((float)(x - plot->offx) / (float)(plot->width)) * 
		(float)(plot->group->xrangemax - plot->group->xrangemin)) + (float)plot->group->xrangemin);
}

float psip_conv_pixel_to_val(PLOT *plot, int y)
{
	struct psipdata *plotdata = (struct psipdata *)plot->plotdata;

	return (((float)plotdata->ymax - (((float)(y - plot->offy) / \
		(float)(plot->height)) * (float)(plotdata->ymax - plotdata->ymin))));
}


/*
** draws the psip in a pixmap
**
** dpy									== needed for XDrawSegments
** pixmap								== must be already created
** gc									== for drawing the line segments
** offx,offy,height,width				== defines a rectangle in which to draw
** samples, nsamples					== the signed short raw-data samples
** xmin,xmax,ymin,ymax					== the axis bounds
*/
static void DrawPSIP(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float *intervals, int nintervals, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, int ymin, int ymax)
{
	XPoint *pts;
	float pixels_per_ms, pixels_per_yrange, winstart, winstop;
	int i, nsegs;
	XRectangle clipbox;

	if ((intervals == NULL) || (nintervals == 0) || (ymin >= ymax) || (xmin >= xmax) || (height < 10))
		return;

	clipbox.x = 0;
	clipbox.y = 0;
	clipbox.width = width;
	clipbox.height = height;
	XSetClipRectangles(dpy, gc, offx, offy, &clipbox, 1, Unsorted);

	winstart = xrangemin;
	winstop = xrangemax;
	pixels_per_ms = ((float)width) / (xrangemax - xrangemin);
	pixels_per_yrange = ((float)height) / ((float)(ymax - ymin));

	if ((pts = (XPoint *)malloc(nintervals * sizeof(XPoint))) != NULL)
	{
		nsegs = 0;
		for (i=0; i < nintervals; i++)
		{
			if ((intervals[2*i] >= winstart) && (intervals[2*i] < winstop))
			{
				pts[nsegs].x = offx + (intervals[2*i] - winstart) * pixels_per_ms;
				pts[nsegs++].y = offy + height - ((intervals[2*i + 1] - ymin) * pixels_per_yrange);
			}
		}
		XDrawLines(dpy, pixmap, gc, pts, nsegs, CoordModeOrigin);
		free(pts);
	}
	XSetClipMask(dpy, gc, None);
}


#define PRINTER_DPI 600.0

static void PSDrawPSIP(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float *intervals, int nintervals, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, int ymin, int ymax)
{
	PSPoint *pts;
	float pixels_per_ms, pixels_per_yrange, winstart, winstop;
	int i, nsegs, newwidth, newheight;

	if ((intervals == NULL) || (nintervals == 0) || (ymin >= ymax) || (xmin >= xmax) || (height < 10))
		return;

	newwidth = width * PRINTER_DPI / 72.0;
	newheight = height * PRINTER_DPI / 72.0;

	winstart = xrangemin;
	winstop = xrangemax;
	pixels_per_ms = ((float)newwidth) / (xrangemax - xrangemin);
	pixels_per_yrange = ((float)newheight) / ((float)(ymax - ymin));

	fprintf(fp, "gsave\n");
	fprintf(fp, "%d %d translate\n", offx, offy2);
	fprintf(fp, "%f %f scale\n", 72.0 / PRINTER_DPI, 72.0 / PRINTER_DPI);
	fprintf(fp, "newpath\n");

	if ((pts = (PSPoint *)malloc(nintervals * sizeof(PSPoint))) != NULL)
	{
		nsegs = 0;
		for (i=0; i < nintervals; i++)
		{
			if ((intervals[2*i] >= winstart) && (intervals[2*i] < winstop))
			{
				pts[nsegs].x = (intervals[2*i] - winstart) * pixels_per_ms;
				pts[nsegs++].y = ((intervals[2*i + 1] - ymin) * pixels_per_yrange);
			}
		}
		PSDrawLinesFP(fp, pts, nsegs);
		free(pts);
	}

	fprintf(fp, "grestore\n");
	fprintf(fp, "closepath fill\n");
}

