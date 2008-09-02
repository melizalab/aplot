
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
struct ccordata
{
	GC drawing_GC, inverse_GC, mark_GC;
	Pixmap pixmap;
	Boolean pixmapalloced;
	Boolean butgrabbed;
	XmFontList lblfont;
	int xmin,xmax;
	float ymin,ymax;

	float *bins;
	int numbins;
	float bindur;
	float start, stop;
	Boolean boxes;
	Boolean normalize;

	float wsize;
	float tau;
	float taustep;
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

#define CreateMenuToggle(widget, parent, name, label, set, userdata) \
	widget = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNset, (set), 															\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNvalueChangedCallback, ccor_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);

#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, ccor_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int ccor_open(PLOT *plot);
int ccor_display(PLOT *plot);
int ccor_set(PLOT *plot);
int ccor_close(PLOT *plot);
int ccor_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int ccor_save(PLOT *plot);
static void ccor_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void ccor_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void ccor_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void ccor_clearmarks(PLOT *plot);
static void ccor_drawstartmark(PLOT *plot, float t);
static void ccor_drawstopmark(PLOT *plot, float t);
static void ccor_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);
static void DrawCCOR(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float ymin, float ymax, boolean boxes);
static void PSDrawCCOR(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float ymin, float ymax, boolean boxes);
static int countspikes(float *toe, int toelen, float start, float end, float *cache, int *cacheval);

float ccor_conv_pixel_to_time(PLOT *plot, int x);
float ccor_conv_pixel_to_val(PLOT *plot, int y);

/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int ccor_open(PLOT *plot)
{
	struct ccordata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12;
	Widget m4a, m4a1, m4a2, m4a3, m4a4, m4a5, m4a6, m4a7, m4a8;
	Widget m9a, m9a1, m9a2, m9a3, m9a4, m9a5, m9a6, m9a7, m9a8;
	Widget m10a, m10a1, m10a2, m10a3, m10a4, m10a5, m10a6, m10a7, m10a8;
	Widget m11a, m11a0, m11a1, m11a2, m11a3, m11a4, m11a5, m11a6, m11a7, m11a8;
	Widget m12a, m12a0, m12a1, m12a2, m12a3, m12a4, m12a5, m12a6, m12a7, m12a8;
	int status = SUCCESS;
	Dimension width, height;
	int depth, offx, offy, offx2, offy2;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct ccordata));
	plotdata = (struct ccordata *)plot->plotdata;

	plotdata->wsize = defaults.ccor_wsize;
	plotdata->tau = defaults.ccor_tau;
	plotdata->taustep = defaults.ccor_taustep;

	plot->plot_widget = XtVaCreateManagedWidget("ccor", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.ccor_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);
	XtAddCallback(plot->plot_widget, XmNexposeCallback, ccor_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, ccor_resize_callback, (XtPointer)plot);
	plot->dirty = 0;
	plotdata->boxes = TRUE;
	plotdata->normalize = TRUE;

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

	plot->drawxaxis = defaults.ccor_drawxaxis;
	plot->drawyaxis = defaults.ccor_drawyaxis;

	offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
	offy = plot->offy = 0;
	offx2 = plot->offx2 = 0;
	offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
	plot->width = width - offx - offx2;
	plot->height = height - offy - offy2;

	plot->depth = depth;

	/*
	** Create the Graphics contexts.
	** drawing_GC is for the ccor itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
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

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m4a = XmCreatePulldownMenu(mw, "m4a", args, n);
	m4 = XtVaCreateManagedWidget("ccorwsize", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("CCOR wsize"),
		XmNsubMenuId, m4a,
		NULL);
	CreateMenuToggle(m4a1, m4a, "ccorwsize1", "1", (plotdata->wsize == 1), "CCOR wsize 1");
	CreateMenuToggle(m4a2, m4a, "ccorwsize2", "2", (plotdata->wsize == 2), "CCOR wsize 2");
	CreateMenuToggle(m4a3, m4a, "ccorwsize3", "3", (plotdata->wsize == 3), "CCOR wsize 3");
	CreateMenuToggle(m4a4, m4a, "ccorwsize4", "4", (plotdata->wsize == 4), "CCOR wsize 4");
	CreateMenuToggle(m4a5, m4a, "ccorwsize5", "5", (plotdata->wsize == 5), "CCOR wsize 5");
	CreateMenuToggle(m4a6, m4a, "ccorwsize6", "10", (plotdata->wsize == 10), "CCOR wsize 10");
	CreateMenuToggle(m4a7, m4a, "ccorwsize7", "15", (plotdata->wsize == 15), "CCOR wsize 15");
	CreateMenuToggle(m4a8, m4a, "ccorwsize8", "20", (plotdata->wsize == 20), "CCOR wsize 20");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m9a = XmCreatePulldownMenu(mw, "m9a", args, n);
	m9 = XtVaCreateManagedWidget("ccortaustep", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("CCOR taustep"),
		XmNsubMenuId, m9a,
		NULL);
	CreateMenuToggle(m9a1, m9a, "ccortaustep1", "1", (plotdata->taustep == 1), "CCOR taustep 1");
	CreateMenuToggle(m9a2, m9a, "ccortaustep2", "2", (plotdata->taustep == 2), "CCOR taustep 2");
	CreateMenuToggle(m9a3, m9a, "ccortaustep3", "3", (plotdata->taustep == 3), "CCOR taustep 3");
	CreateMenuToggle(m9a4, m9a, "ccortaustep4", "4", (plotdata->taustep == 4), "CCOR taustep 4");
	CreateMenuToggle(m9a5, m9a, "ccortaustep5", "5", (plotdata->taustep == 5), "CCOR taustep 5");
	CreateMenuToggle(m9a6, m9a, "ccortaustep6", "10", (plotdata->taustep == 10), "CCOR taustep 10");
	CreateMenuToggle(m9a7, m9a, "ccortaustep7", "15", (plotdata->taustep == 15), "CCOR taustep 15");
	CreateMenuToggle(m9a8, m9a, "ccortaustep8", "20", (plotdata->taustep == 20), "CCOR taustep 20");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m10a = XmCreatePulldownMenu(mw, "m10a", args, n);
	m10 = XtVaCreateManagedWidget("ccortau", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("CCOR tau"),
		XmNsubMenuId, m10a,
		NULL);
	CreateMenuToggle(m10a1, m10a, "ccortau1", "10", (plotdata->tau == 10), "CCOR tau 10");
	CreateMenuToggle(m10a2, m10a, "ccortau2", "20", (plotdata->tau == 20), "CCOR tau 20");
	CreateMenuToggle(m10a3, m10a, "ccortau3", "30", (plotdata->tau == 30), "CCOR tau 30");
	CreateMenuToggle(m10a4, m10a, "ccortau4", "40", (plotdata->tau == 40), "CCOR tau 40");
	CreateMenuToggle(m10a5, m10a, "ccortau5", "50", (plotdata->tau == 50), "CCOR tau 50");
	CreateMenuToggle(m10a6, m10a, "ccortau6", "100", (plotdata->tau == 100), "CCOR tau 100");
	CreateMenuToggle(m10a7, m10a, "ccortau7", "150", (plotdata->tau == 150), "CCOR tau 150");
	CreateMenuToggle(m10a8, m10a, "ccortau8", "200", (plotdata->tau == 200), "CCOR tau 200");

	CreateMenuToggle(m5, mw, "boxes", "Boxes", plotdata->boxes, "Boxes");
	CreateMenuToggle(m6, mw, "normalize", "Normalize", plotdata->normalize, "Normalize");
	CreateMenuButton(m7, mw, "save", "Save", "Save");
	CreateMenuButton(m8, mw, "print", "Print EPS", "Print EPS");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m11a = XmCreatePulldownMenu(mw, "m11a", args, n);
	m11 = XtVaCreateManagedWidget("aunit", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Unit A"),
		XmNsubMenuId, m11a,
		NULL);
	CreateMenuToggle(m11a0, m11a, "aunit0", "All", (plot->group->toe_curunit == 0), "AUnit 0");
	CreateMenuToggle(m11a1, m11a, "aunit1", "1", (plot->group->toe_curunit == 1), "AUnit 1");
	CreateMenuToggle(m11a2, m11a, "aunit2", "2", (plot->group->toe_curunit == 2), "AUnit 2");
	CreateMenuToggle(m11a3, m11a, "aunit3", "3", (plot->group->toe_curunit == 3), "AUnit 3");
	CreateMenuToggle(m11a4, m11a, "aunit4", "4", (plot->group->toe_curunit == 4), "AUnit 4");
	CreateMenuToggle(m11a5, m11a, "aunit5", "5", (plot->group->toe_curunit == 5), "AUnit 5");
	CreateMenuToggle(m11a6, m11a, "aunit6", "6", (plot->group->toe_curunit == 6), "AUnit 6");
	CreateMenuToggle(m11a7, m11a, "aunit7", "7", (plot->group->toe_curunit == 7), "AUnit 7");
	CreateMenuToggle(m11a8, m11a, "aunit8", "8", (plot->group->toe_curunit == 8), "AUnit 8");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m12a = XmCreatePulldownMenu(mw, "m12a", args, n);
	m12 = XtVaCreateManagedWidget("bunit", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Unit B"),
		XmNsubMenuId, m12a,
		NULL);
	CreateMenuToggle(m12a0, m12a, "bunit0", "All", (plot->group->toe2_curunit == 0), "BUnit 0");
	CreateMenuToggle(m12a1, m12a, "bunit1", "1", (plot->group->toe2_curunit == 1), "BUnit 1");
	CreateMenuToggle(m12a2, m12a, "bunit2", "2", (plot->group->toe2_curunit == 2), "BUnit 2");
	CreateMenuToggle(m12a3, m12a, "bunit3", "3", (plot->group->toe2_curunit == 3), "BUnit 3");
	CreateMenuToggle(m12a4, m12a, "bunit4", "4", (plot->group->toe2_curunit == 4), "BUnit 4");
	CreateMenuToggle(m12a5, m12a, "bunit5", "5", (plot->group->toe2_curunit == 5), "BUnit 5");
	CreateMenuToggle(m12a6, m12a, "bunit6", "6", (plot->group->toe2_curunit == 6), "BUnit 6");
	CreateMenuToggle(m12a7, m12a, "bunit7", "7", (plot->group->toe2_curunit == 7), "BUnit 7");
	CreateMenuToggle(m12a8, m12a, "bunit8", "8", (plot->group->toe2_curunit == 8), "BUnit 8");

	/*
	** Register an event handler
	*/
	XtAddEventHandler(w, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
		FALSE, (XtEventHandler) ccor_event_handler, (XtPointer)plot);

	plot->plot_clearmarks = NULL;
	plot->plot_drawstartmark = NULL;
	plot->plot_drawstopmark = NULL;

	plot->plot_display = ccor_display;
	plot->plot_set = ccor_set;
	plot->plot_close = ccor_close;
	plot->plot_print = ccor_print;
	plot->plot_save = ccor_save;

	plot->plot_event_handler = ccor_event_handler;

	plotdata->butgrabbed = FALSE;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needtoe = 1;

	plot->plot_clearmarks = ccor_clearmarks;
	plot->plot_drawstartmark = ccor_drawstartmark;
	plot->plot_drawstopmark = ccor_drawstopmark;
	plot->plot_conv_pixel_to_time = ccor_conv_pixel_to_time;

	return status;
}

int ccor_display(PLOT *plot)
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
			if (load_toedata(&group->toe2fp, group->loadedfilename2, group->filename2, group->entry, group->toe2_curunit,
				&group->toe2_datacount, &group->toe2_nunits, &group->toe2_nreps,
				&group->toe2_repcounts, &group->toe2_repptrs, &group->start, &group->stop) == 0)
			{
				group->toedirty = 0;
			}
			group->xrangemin = group->start;
			group->xrangemax = group->stop;
		}
		else status = ERROR;
	}

	tstatus = ccor_set(plot);
	return (status == SUCCESS) ? (tstatus) : (status);
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
int ccor_set(PLOT *plot)
{
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	int rep, i, j, xmin, xmax, offx, offy, offx2, offy2;
	float *bins, ymin, ymax;
	int numbins, count1, count2;
	float firstbinstart, lastbinstop, startwindow, endwindow, tauindex, tau, wsize, taustep;
	float *toe1, *toe2, cache1, cache2;
	int toe1len, toe2len, cacheval1, cacheval2;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to ccor_open().
	*/
	if (plotdata->butgrabbed == FALSE)
	{
		XGrabButton(dpy, AnyButton, AnyModifier, XtWindow(w), TRUE,
			ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
			GrabModeAsync, GrabModeAsync, XtWindow(w),
			XCreateFontCursor(dpy, XC_crosshair));
		plotdata->butgrabbed = TRUE;
	}

	/*
	** Sometimes we get called from the resize callback before we are ready...
	*/
	if (plot->group->samplerate == 0)
		return SUCCESS;

	if (plotdata->pixmapalloced)
	{
		/*
		** Retrieve the window size
		*/
		XtVaGetValues(w,
			XmNheight, &height,
			XmNwidth, &width,
			NULL);
		defaults.ccor_height = height;
		defaults.width = width;

		/*
		** Set the axes margins (note: if you want to change
		** the ticklblfont, then you must also update the
		** plot->minoff?? numbers.  They are not recalculated
		** here.)  (see ccor_open)
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
		** Calculate the bounds
		*/
		xmin = plot->group->xrangemin * (plot->group->samplerate / 1000.0);
		xmax = plot->group->xrangemax * (plot->group->samplerate / 1000.0);

		/*
		** Calculate the histogram
		** NOTE: only do this if it needs to be done (ie: it hasn't been calculated already OR
		** something affecting the calculation has changed).
		*/
		tau = plotdata->tau;
		taustep = plotdata->taustep;
		wsize = plotdata->wsize;
		ymin = plotdata->ymin;
		ymax = plotdata->ymax;
		bins = plotdata->bins;
		numbins = plotdata->numbins;

		if ((plot->group->dirty == 1) ||
			(plot->dirty == 1) || 
			(plotdata->xmin != xmin) ||
			(plotdata->xmax != xmax) ||
			(plotdata->bins == NULL))
		{
			plot->dirty = 0;
			if (plotdata->bins != NULL)
				free(plotdata->bins);
			bins = plotdata->bins = NULL;

			plotdata->xmin = xmin;
			plotdata->xmax = xmax;

			numbins = plotdata->numbins = (2 * tau / taustep) + 1;
			if (numbins > 0)
			{
				firstbinstart = plot->group->xrangemin;
				lastbinstop = plot->group->xrangemax;
				if ((bins = (float *)calloc(numbins + 1, sizeof(float))) != NULL)
				{
					if (plot->group->toe_nreps == plot->group->toe2_nreps)
					{
						for (rep=1; rep <= plot->group->toe_nreps; rep++)
						{
							toe1 = plot->group->toe_repptrs[rep];
							toe1len = plot->group->toe_repcounts[rep];
							while ((toe1len > 0) && (*toe1 < firstbinstart)) { toe1++; toe1len--; };
							while ((toe1len > 0) && (toe1[toe1len-1] >= lastbinstop)) { toe1len--; };

							toe2 = plot->group->toe2_repptrs[rep];
							toe2len = plot->group->toe2_repcounts[rep];
							while ((toe2len > 0) && (*toe2 < firstbinstart)) { toe2++; toe2len--; };
							while ((toe2len > 0) && (toe2[toe2len-1] >= lastbinstop)) { toe2len--; };

							if ((toe1len == 0) || (toe2len == 0))
								continue;

							/*
							** Loop through each spike in toe1
							*/
							cache1 = toe1[toe1len - 1] + 1;
							cache2 = toe2[toe2len - 1] + 1;
							for (j=0; j < toe1len; j++)
							{
								startwindow = toe1[j] - wsize;
								endwindow = toe1[j] + wsize;
								count1 = countspikes(toe1, toe1len, startwindow, endwindow, &cache1, &cacheval1);

								/* Loop through index shifts from -tau:taustep:tau */
								for (tauindex = -1 * tau, i=0; i < numbins; i++, tauindex += taustep)
								{
									count2 = countspikes(toe2, toe2len, startwindow + tauindex, endwindow + tauindex, &cache2, &cacheval2);
									bins[i] += count1 * count2;
								}
							}
						}
					}
					else
					{
						fprintf(stderr, "Error: Can't cross correlate toe lists w/ diff. #'s of reps!\n");
					}
				}
				for (ymax=50.0, i=0; i < numbins; i++)
					if (ymax < bins[i]) ymax = bins[i];
				ymax++;
				plotdata->ymax = ymax;
				plotdata->bins = bins;
			}
		}

		/*
		** Clear any marks
		*/
		plot->markx1 = -1;
		plot->markx2 = -1;

		/*
		** Do the rendering to an off-screen pixmap.
		*/
		DrawCCOR(dpy, plotdata->pixmap, plotdata->drawing_GC, offx, offy, width, height,
			bins, numbins, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			-1 * (plotdata->tau * plot->group->samplerate / 1000),
			plotdata->tau * (plot->group->samplerate / 1000),
			ymin, ymax, plotdata->boxes);

		if (plot->drawxaxis == TRUE)
			DrawXAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
				offx, offy, width, height,
				-1 * plotdata->tau * plot->group->samplerate / 1000,
				plotdata->tau * plot->group->samplerate / 1000);

		if (plot->drawyaxis == TRUE)
			DrawYAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
				offx, offy, width, height, ymin, ymax);

		XCopyArea(dpy, plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
			0, 0, width + offx + offx2, height + offy + offy2, 0, 0);
	}
	return SUCCESS;
}

int ccor_close(PLOT *plot)
{
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;
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

int ccor_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;
	Dimension width, height;
	char filenamestore[256], *filename, *ch, tempstr[40];
	int xmin, xmax, offx, offy, offx2, offy2;
	float *bins, ymin, ymax;
	int numbins;
	int fileopenflag = 0;

	/*
	** Create the filename of the output file:
	** 1) start with the input filename
	** 2) remove the extension
	** 3) add the extension: _ccor_<entry>_<startms>_<stopms>.eps
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->filename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_ccor_%ld_%d_%d.eps", plot->group->entry, (int)plot->group->xrangemin, (int)plot->group->xrangemax);
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
		bins = plotdata->bins;
		numbins = plotdata->numbins;
		PSDrawCCOR(fp, offx, offy, offx2, offy2, width, height,
			bins, numbins, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			-1 * plotdata->tau * plot->group->samplerate / 1000,
			plotdata->tau * plot->group->samplerate / 1000, ymin, ymax, plotdata->boxes);
		if (plot->drawxaxis == TRUE)
			PSDrawXAxis(fp, offx, offy, offx2, offy2, width, height,
				-1 * plotdata->tau * plot->group->samplerate / 1000,
				plotdata->tau * plot->group->samplerate / 1000);
		if (plot->drawyaxis == TRUE)
			PSDrawYAxis(fp, offx, offy, offx2, offy2, width, height, ymin, ymax);

		PSFinish(fp);
		if (fileopenflag == 1)
			fclose(fp);
	}
	if (ret_filename_p) *ret_filename_p = strdup(filename);
	return SUCCESS;
}

int ccor_save(PLOT *plot)
{
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;
	FILE *fp;
	char filenamestore[256], *filename, *ch, tempstr[40];
	float tmin, tmax, ttmp;
	int i;

	if ((plot->markx1 != -1) && (plot->markx2 != -1))
	{
		tmin = ccor_conv_pixel_to_time(plot, plot->markx1);
		tmax = ccor_conv_pixel_to_time(plot, plot->markx2);
		if (tmin > tmax)
		{
			ttmp = tmax;
			tmax = tmin;
			tmin = ttmp;
		}
	}
	else 
	{
		tmin = plot->group->xrangemin;
		tmax = plot->group->xrangemax;
	}

	/*
	** Create the filename of the output file:
	** 1) start with the input filename
	** 2) remove the extension
	** 3) add the extension: _ccor_<entry>_<startms>_<stopms>.dat
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->filename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_ccor_%ld_%d_%d.dat", plot->group->entry,
		(int) tmin,
		(int) tmax);
	strcat(filename, tempstr);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '/')) ch--;
	if (*ch == '/') filename = ++ch;

	/*
	** 
	*/
	if ((plotdata->bins != NULL) && (plotdata->numbins > 0))
	{
		if ((fp = fopen(filename, "w")) != NULL)
		{
			for (i=0; i < plotdata->numbins; i++)
				fprintf(fp, "%f\n", plotdata->bins[i]);
			printf("CCOR: saved %d histogram values into '%s'\n", plotdata->numbins, filename);
			fclose(fp);
		}
	}
	return SUCCESS;
}


static void ccor_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;
	XExposeEvent *expevent;
	int x, y0,y1;

	/*
	** Take care of the actual ccor.  Then,
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
static void ccor_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	plot->dirty = 1;
	ccor_set(plot);
	return;
}

static void ccor_clearmarks(PLOT *plot)
{
	if (plot->markx1 != -1)
		plot->markx1 = -1;
	if (plot->markx2 != -1)
		plot->markx2 = -1;
}

static void ccor_drawstartmark(PLOT *plot, float t)
{
	int x;

	if (plot->group->xrangemin >= plot->group->xrangemax)
		return;

	x = CONV_MS_TO_X(plot, t);
	if (IS_X_INBOUNDS(plot, x))
		plot->markx1 = x;
}

static void ccor_drawstopmark(PLOT *plot, float t)
{
	int x;

	if (plot->group->xrangemin >= plot->group->xrangemax)
		return;

	x = CONV_MS_TO_X(plot, t);
	if (x != plot->markx2)								/* If there's no change, then do nothing */
	{
		if (IS_X_INBOUNDS(plot, x))
			plot->markx2 = x;
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
static void ccor_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
{
	PLOT *plot = (PLOT *)clientData;
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;
	XButtonEvent *butevent;
	XMotionEvent *motevent;
	XKeyEvent *keyevent;
	char *keybuf;
	KeySym key_sym;
	Modifiers mod_return;
	Widget ew;

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
						plot->dirty = 1;
						plot->drawxaxis = !(plot->drawxaxis);
						defaults.ccor_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "xaxis"), XmNset, plot->drawxaxis, NULL);
						ccor_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'y'))
					{
						plot->dirty = 1;
						plot->drawyaxis = !(plot->drawyaxis);
						defaults.ccor_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "yaxis"), XmNset, plot->drawyaxis, NULL);
						ccor_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'n'))
					{
						plot->dirty = 1;
						plotdata->normalize = ! (plotdata->normalize);
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "normalize"), XmNset, plotdata->normalize, NULL);
						ccor_set(plot);
					}
					else if (KEY_IS_DOWN(keybuf)) /* Keep this here to override the panel trying to zoom in */
					{
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
					break;

				case Button2:
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
					break;
				case Button2:
					break;
				case Button3:
					break;
				default:
					break;
			}
			break;

		case MotionNotify:
			motevent = (XMotionEvent *)event;
			break;

		default:
			break;
	}
}

static void ccor_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;
	char *idstring;
	Boolean drawxaxis, drawyaxis, value, drawboxes, donormalize;
	float wsize, tau, taustep;
	int unit;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "X axis"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &drawxaxis, NULL);
		plot->drawxaxis = drawxaxis;
		defaults.ccor_drawyaxis = plot->drawyaxis;
		ccor_set(plot);
	}
	else if (!strcmp(idstring, "Y axis"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &drawyaxis, NULL);
		plot->drawyaxis = drawyaxis;
		defaults.ccor_drawyaxis = plot->drawyaxis;
		ccor_set(plot);
	}
	else if (!strncmp(idstring, "CCOR taustep", 12))
	{
		plot->dirty = 1;
		sscanf(idstring, "CCOR taustep %f", &taustep);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->taustep != taustep))
		{
			plotdata->taustep = taustep;
			defaults.ccor_taustep = taustep;
			ccor_display(plot);
		}
	}
	else if (!strncmp(idstring, "CCOR tau", 8))
	{
		plot->dirty = 1;
		sscanf(idstring, "CCOR tau %f", &tau);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->tau != tau))
		{
			plotdata->tau = tau;
			defaults.ccor_tau = tau;
			ccor_display(plot);
		}
	}
	else if (!strncmp(idstring, "CCOR wsize", 10))
	{
		plot->dirty = 1;
		sscanf(idstring, "CCOR wsize %f", &wsize);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->wsize != wsize))
		{
			plotdata->wsize = wsize;
			defaults.ccor_wsize = wsize;
			ccor_display(plot);
		}
	}
	else if (!strncmp(idstring, "AUnit", 5))
	{
		plot->dirty = 1;
		plot->group->toedirty = 1;
		sscanf(idstring, "AUnit %d", &unit);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plot->group->toe_curunit != unit))
		{
			plot->group->toe_curunit = unit;
			ccor_display(plot);
		}
	}
	else if (!strncmp(idstring, "BUnit", 5))
	{
		plot->dirty = 1;
		plot->group->toedirty = 1;
		sscanf(idstring, "BUnit %d", &unit);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plot->group->toe2_curunit != unit))
		{
			plot->group->toe2_curunit = unit;
			ccor_display(plot);
		}
	}
	else if (!strcmp(idstring, "Boxes"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &drawboxes, NULL);
		plotdata->boxes = drawboxes;
		ccor_set(plot);
	}
	else if (!strcmp(idstring, "Normalize"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &donormalize, NULL);
		plotdata->normalize = donormalize;
		ccor_set(plot);
	}
	else if (!strcmp(idstring, "Save"))
	{
		ccor_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		ccor_print(plot, NULL, NULL);
	}
}

/*
** For the following, I employ the following notation:
** X, Y refer to pixel coordinates.
** TIME, VAL are in milliseconds and correlation units, respectively.
** SAMPLE is time in samples
*/
float ccor_conv_pixel_to_time(PLOT *plot, int x)
{
	return ((((float)(x - plot->offx) / (float)(plot->width)) * 
			(float)(plot->group->xrangemax - plot->group->xrangemin)) + (float)plot->group->xrangemin);
}

float ccor_conv_pixel_to_val(PLOT *plot, int y)
{
	struct ccordata *plotdata = (struct ccordata *)plot->plotdata;

	return (((float)plotdata->ymax - (((float)(y - plot->offy) / \
		(float)(plot->height)) * (float)(plotdata->ymax - plotdata->ymin))));
}

/*
** draws the ccor in a pixmap
**
** dpy									== needed for XDrawSegments
** pixmap								== must be already created
** gc									== for drawing the line segments
** offx,offy,height,width				== defines a rectangle in which to draw
** samples, nsamples					== the signed short raw-data samples
** xmin,xmax,ymin,ymax					== the axis bounds
*/
static void DrawCCOR(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float ymin, float ymax, boolean boxes)
{
	XPoint *pts;
	XSegment *segs;
	float pixels_per_bin, pixels_per_yrange, x0, x1;
	int i, nsegs, j;
	float binmin, binmax;
	XRectangle clipbox;

	if ((bins == NULL) || (numbins == 0) || (ymin >= ymax) || (xmin >= xmax) || (height < 10))
		return;

	clipbox.x = 0;
	clipbox.y = 0;
	clipbox.width = width;
	clipbox.height = height;
	XSetClipRectangles(dpy, gc, offx, offy, &clipbox, 1, Unsorted);

	pixels_per_yrange = ((float)height) / ((float)(ymax - ymin));
	pixels_per_bin = ((float)width) / ((float)(numbins));

	if (numbins > width)
	{
		nsegs = width;
		if ((segs = (XSegment *)malloc(nsegs * sizeof(XSegment))) != NULL)
		{
			for (i=0; i < width; i++)
			{
				j = (int)(((float)i / (float)width) * (numbins - 1.0));
				binmin = binmax = bins[j];
				while (j < ((int)(((float)(i+1) / (float)width) * (numbins - 1.0))))
				{
					if (bins[j] > binmax) binmax = bins[j];
					if (bins[j] < binmin) binmin = bins[j];
					j++;
				}
				segs[i].x1 = offx + i;
				segs[i].x2 = offx + i;
				segs[i].y1 = offy + height - binmin * pixels_per_yrange;
				segs[i].y2 = offy + height - binmax * pixels_per_yrange;
			}
			XDrawSegments(dpy, pixmap, gc, segs, nsegs);
			free(segs);
		}
	}
	else
	{
		if (boxes == FALSE)
		{
			nsegs = 2 * numbins;
			if ((pts = (XPoint *)malloc(nsegs * sizeof(XPoint))) != NULL)
			{
				for (i=0; i < numbins; i++)
				{
					pts[2*i].x = offx + (i * pixels_per_bin);
					pts[2*i + 1].x = offx + ((i+1) * pixels_per_bin);
					pts[2*i].y = pts[2*i + 1].y = offy + height - bins[i] * pixels_per_yrange;
				}
				XDrawLines(dpy, pixmap, gc, pts, nsegs, CoordModeOrigin);
				free(pts);
			}
		}
		else
		{
			nsegs = 4 * numbins;
			if ((pts = (XPoint *)malloc(nsegs * sizeof(XPoint))) != NULL)
			{
				for (i=0; i < numbins; i++)
				{
					x0 = offx + (i * pixels_per_bin);
					x1 = offx + ((i+1) * pixels_per_bin);
					pts[4*i].x = x0;
					pts[4*i + 1].x = x0;
					pts[4*i + 2].x = x1;
					pts[4*i + 3].x = x1;
					pts[4*i].y = offy + height;
					pts[4*i + 1].y = offy + height - bins[i] * pixels_per_yrange;
					pts[4*i + 2].y = offy + height - bins[i] * pixels_per_yrange;
					pts[4*i + 3].y = offy + height;
				}
				XDrawLines(dpy, pixmap, gc, pts, nsegs, CoordModeOrigin);
				free(pts);
			}
		}
	}

	XSetClipMask(dpy, gc, None);
}


#define PRINTER_DPI 600.0

static void PSDrawCCOR(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float ymin, float ymax, boolean boxes)
{
	PSPoint *pts;
	PSSegment *segs;
	float pixels_per_yrange, pixels_per_bin, x0, x1;
	int i, nsegs, j, newwidth, newheight;
	float binmin, binmax;

	if ((bins == NULL) || (numbins == 0) || (ymin >= ymax) || (xmin >= xmax) || (height < 10))
		return;

	newwidth = width * PRINTER_DPI / 72.0;
	newheight = height * PRINTER_DPI / 72.0;

	fprintf(fp, "gsave\n");
	fprintf(fp, "%d %d translate\n", offx, offy2);
	fprintf(fp, "%f %f scale\n", 72.0 / PRINTER_DPI, 72.0 / PRINTER_DPI);

	pixels_per_bin = ((float)newwidth) / ((float)(numbins));
	pixels_per_yrange = ((float)newheight) / ((float)(ymax - ymin));

	fprintf(fp, "newpath\n");
	if (pixels_per_bin < 0.1)
	{
		nsegs = newwidth;
		if ((segs = (PSSegment *)malloc(nsegs * sizeof(PSSegment))) != NULL)
		{
			for (i=0; i < nsegs; i++)
			{
				j = (int)(((float)i / (float)nsegs) * (numbins - 1.0));
				binmin = binmax = bins[j];
				while (j < ((int)(((float)(i+1) / (float)nsegs) * (numbins - 1.0))))
				{
					if (bins[j] > binmax) binmax = bins[j];
					if (bins[j] < binmin) binmin = bins[j];
					j++;
				}
				segs[i].x1 = segs[i].x2 = i;
				segs[i].y1 = binmin * pixels_per_yrange;
				segs[i].y2 = binmax * pixels_per_yrange;
			}
			PSDrawSegmentsFP(fp, segs, nsegs);
			free(segs);
		}
	}
	else
	{
		if (boxes == FALSE)
		{
			nsegs = 2 * numbins;
			if ((pts = (PSPoint *)malloc(nsegs * sizeof(PSPoint))) != NULL)
			{
				for (i=0; i < numbins; i++)
				{
					pts[2*i].x = i * pixels_per_bin;
					pts[2*i + 1].x = (i+1) * pixels_per_bin;
					pts[2*i].y = pts[2*i + 1].y = bins[i] * pixels_per_yrange;
				}
				PSDrawLinesFP(fp, pts, nsegs);
				free(pts);
			}
		}
		else
		{
			nsegs = 4 * numbins;
			if ((pts = (PSPoint *)malloc(nsegs * sizeof(PSPoint))) != NULL)
			{
				for (i=0; i < numbins; i++)
				{
					x0 = i * pixels_per_bin;
					x1 = (i+1) * pixels_per_bin;
					pts[4*i].x = x0;
					pts[4*i + 1].x = x0;
					pts[4*i + 2].x = x1;
					pts[4*i + 3].x = x1;
					pts[4*i].y = 0;
					pts[4*i + 1].y = bins[i] * pixels_per_yrange;
					pts[4*i + 2].y = bins[i] * pixels_per_yrange;
					pts[4*i + 3].y = 0;
				}
				PSDrawLinesFP(fp, pts, nsegs);
				free(pts);
			}
		}
	}
	fprintf(fp, "grestore\n");
	fprintf(fp, "closepath fill\n");
}


static int countspikes(float *toe, int toelen, float start, float end, float *cache, int *cacheval)
{
	int i, j;

	/* Find first event >= start */

	/* Check the cache to avoid scanning hundreds of events unnecessarily */
	if (start > *cache)
		i = *cacheval;
	else
		i = 0;

	/* If the last event is not >= start, then don't bother scanning */
	if (toe[toelen - 1] < start)
		i = toelen;
	else while (toe[i] < start) i++;

	/* Cache the first event >= start, so we won't scan 0 to i if start > *cache */
	*cache = start;
	*cacheval = i;

	/* Find first event after above that is > end */
	j = i;
	while ((j < toelen) && (toe[j] <= end))
		j++;

	return (j - i);
}

