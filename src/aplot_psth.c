
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
struct psthdata
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
	Boolean filled;
	Boolean ymax_override;
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
	XtAddCallback(widget, XmNvalueChangedCallback, psth_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);

#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, psth_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int psth_open(PLOT *plot);
int psth_display(PLOT *plot);
int psth_set(PLOT *plot);
int psth_close(PLOT *plot);
int psth_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int psth_save(PLOT *plot);
static void psth_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void psth_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void psth_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void psth_clearmarks(PLOT *plot);
static void psth_drawstartmark(PLOT *plot, float t);
static void psth_drawstopmark(PLOT *plot, float t);
static void psth_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);
static void DrawPSTH(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	float ymin, float ymax, boolean boxes, boolean filled);
static void PSDrawPSTH(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	float ymin, float ymax, boolean boxes, boolean filled);

float psth_conv_pixel_to_time(PLOT *plot, int x);
float psth_conv_pixel_to_val(PLOT *plot, int y);


/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int psth_open(PLOT *plot)
{
	struct psthdata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11;
	Widget m4a, m4a1, m4a2, m4a3, m4a4, m4a5, m4a6, m4a7, m4a8, m4a9;
	Widget m9a, m9a1, m9a2, m9a3, m9a4, m9a5, m9a6, m9a7, m9a8;
	Widget m10a, m10a1, m10a2, m10a3, m10a4, m10a5, m10a6, m10a7, m10a8, m10a9, m10a10, m10a11, m10a12;
	int status = SUCCESS;
	Dimension width, height;
	int depth, offx, offy, offx2, offy2;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct psthdata));
	plotdata = (struct psthdata *)plot->plotdata;

	plot->plot_widget = XtVaCreateManagedWidget("psth", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.psth_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);
	XtAddCallback(plot->plot_widget, XmNexposeCallback, psth_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, psth_resize_callback, (XtPointer)plot);
	plot->dirty = 0;
	plotdata->boxes = TRUE;
	plotdata->filled = TRUE;
	plotdata->ymax = defaults.psth_ymax;
	plotdata->ymax_override = (plotdata->ymax >= 0);
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

	plot->drawxaxis = defaults.psth_drawxaxis;
	plot->drawyaxis = defaults.psth_drawyaxis;

	offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
	offy = plot->offy = 0;
	offx2 = plot->offx2 = 0;
	offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
	plot->width = width - offx - offx2;
	plot->height = height - offy - offy2;

	plot->depth = depth;

	/*
	** Create the Graphics contexts.
	** drawing_GC is for the psth itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
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
	m4 = XtVaCreateManagedWidget("psthbindur", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("PSTH bindur"),
		XmNsubMenuId, m4a,
		NULL);
	CreateMenuToggle(m4a1, m4a, "psthbindur1", "1", (plot->group->psth_bindur == 1), "PSTH bindur 1");
	CreateMenuToggle(m4a2, m4a, "psthbindur2", "2", (plot->group->psth_bindur == 2), "PSTH bindur 2");
	CreateMenuToggle(m4a3, m4a, "psthbindur3", "3", (plot->group->psth_bindur == 3), "PSTH bindur 3");
	CreateMenuToggle(m4a4, m4a, "psthbindur4", "4", (plot->group->psth_bindur == 4), "PSTH bindur 4");
	CreateMenuToggle(m4a5, m4a, "psthbindur5", "5", (plot->group->psth_bindur == 5), "PSTH bindur 5");
	CreateMenuToggle(m4a6, m4a, "psthbindur6", "10", (plot->group->psth_bindur == 10), "PSTH bindur 10");
	CreateMenuToggle(m4a7, m4a, "psthbindur7", "15", (plot->group->psth_bindur == 15), "PSTH bindur 15");
	CreateMenuToggle(m4a8, m4a, "psthbindur8", "20", (plot->group->psth_bindur == 20), "PSTH bindur 20");
	CreateMenuToggle(m4a9, m4a, "psthbindur9", "50", (plot->group->psth_bindur == 50), "PSTH bindur 50");

	CreateMenuToggle(m5, mw, "boxes", "Boxes", plotdata->boxes, "Boxes");
	CreateMenuToggle(m6, mw, "normalize", "Normalize", plotdata->normalize, "Normalize");
	CreateMenuButton(m7, mw, "save", "Save", "Save");
	CreateMenuButton(m8, mw, "print", "Print EPS", "Print EPS");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m9a = XmCreatePulldownMenu(mw, "m9a", args, n);
	m9 = XtVaCreateManagedWidget("unit", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Unit"),
		XmNsubMenuId, m9a,
		NULL);
	CreateMenuToggle(m9a1, m9a, "unit1", "1", (plot->group->toe_curunit == 1), "Unit 1");
	CreateMenuToggle(m9a2, m9a, "unit2", "2", (plot->group->toe_curunit == 2), "Unit 2");
	CreateMenuToggle(m9a3, m9a, "unit3", "3", (plot->group->toe_curunit == 3), "Unit 3");
	CreateMenuToggle(m9a4, m9a, "unit4", "4", (plot->group->toe_curunit == 4), "Unit 4");
	CreateMenuToggle(m9a5, m9a, "unit5", "5", (plot->group->toe_curunit == 5), "Unit 5");
	CreateMenuToggle(m9a6, m9a, "unit6", "6", (plot->group->toe_curunit == 6), "Unit 6");
	CreateMenuToggle(m9a7, m9a, "unit7", "7", (plot->group->toe_curunit == 7), "Unit 7");
	CreateMenuToggle(m9a8, m9a, "unit8", "8", (plot->group->toe_curunit == 8), "Unit 8");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m10a = XmCreatePulldownMenu(mw, "m10a", args, n);
	m10 = XtVaCreateManagedWidget("psthbindur", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Ymax"),
		XmNsubMenuId, m10a,
		NULL);
#define YMAX_EQUALS_N(a) (ABS(plotdata->ymax - (a)) < 0.001)
	CreateMenuToggle(m10a1, m10a, "ymax1", "default", YMAX_EQUALS_N(-1.0), "Ymax -1");
	CreateMenuToggle(m10a2, m10a, "ymax2", "1", YMAX_EQUALS_N(1), "Ymax 1");
	CreateMenuToggle(m10a3, m10a, "ymax3", "5", YMAX_EQUALS_N(5), "Ymax 5");
	CreateMenuToggle(m10a4, m10a, "ymax4", "10", YMAX_EQUALS_N(10), "Ymax 10");
	CreateMenuToggle(m10a5, m10a, "ymax5", "20", YMAX_EQUALS_N(20), "Ymax 20");
	CreateMenuToggle(m10a6, m10a, "ymax6", "30", YMAX_EQUALS_N(30), "Ymax 30");
	CreateMenuToggle(m10a7, m10a, "ymax7", "40", YMAX_EQUALS_N(40), "Ymax 40");
	CreateMenuToggle(m10a8, m10a, "ymax8", "50", YMAX_EQUALS_N(50), "Ymax 50");
	CreateMenuToggle(m10a9, m10a, "ymax9", "75", YMAX_EQUALS_N(75), "Ymax 75");
	CreateMenuToggle(m10a10, m10a, "ymax10", "100", YMAX_EQUALS_N(100), "Ymax 100");
	CreateMenuToggle(m10a11, m10a, "ymax11", "200", YMAX_EQUALS_N(200), "Ymax 200");
	CreateMenuToggle(m10a12, m10a, "ymax12", "300", YMAX_EQUALS_N(300), "Ymax 300");
#undef YMAX_EQUALS_N

	CreateMenuToggle(m11, mw, "filled", "Filled", plotdata->filled, "Filled");

	/*
	** Register an event handler
	*/
	XtAddEventHandler(w, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
		FALSE, (XtEventHandler) psth_event_handler, (XtPointer)plot);

	plot->plot_clearmarks = psth_clearmarks;
	plot->plot_drawstartmark = psth_drawstartmark;
	plot->plot_drawstopmark = psth_drawstopmark;
	plot->plot_conv_pixel_to_time = psth_conv_pixel_to_time;

	plot->plot_display = psth_display;
	plot->plot_set = psth_set;
	plot->plot_close = psth_close;
	plot->plot_print = psth_print;
	plot->plot_save = psth_save;

	plot->plot_event_handler = psth_event_handler;

	plotdata->butgrabbed = FALSE;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needtoe = 1;

	return status;
}

int psth_display(PLOT *plot)
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

	if (plot->group->psth_bindur == 0.0)
		plot->group->psth_bindur = 5.0;

	tstatus = psth_set(plot);
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
int psth_set(PLOT *plot)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	int i, j, xmin, xmax, offx, offy, offx2, offy2;
	float *bins, ymin, ymax;
	int bin, numbins, count;
	float *data, bindur, firstbinstart, lastbinstop;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to psth_open().
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
		defaults.psth_height = height;
		defaults.width = width;

		/*
		** Set the axes margins (note: if you want to change
		** the ticklblfont, then you must also update the
		** plot->minoff?? numbers.  They are not recalculated
		** here.)  (see psth_open)
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

		/*
		** Calculate the histogram
		** NOTE: only do this if it needs to be done (ie: it hasn't been calculated already OR
		** something affecting the calculation has changed).
		*/
		if ((plot->group->dirty == 1) ||
			(plot->dirty == 1) || 
			(plotdata->bindur != plot->group->psth_bindur) ||
			(plotdata->xmin != xmin) ||
			(plotdata->xmax != xmax) ||
			(plotdata->start != plot->group->start) ||
			(plotdata->stop != plot->group->stop) ||
			(plotdata->bins == NULL))
		{
			plot->dirty = 0;
			plotdata->bindur = bindur = plot->group->psth_bindur;
			numbins = (plot->group->xrangemax - plot->group->xrangemin) / bindur;
			if (plotdata->bins != NULL) free(plotdata->bins);
			plotdata->bins = bins = NULL;
			plotdata->xmin = xmin;
			plotdata->xmax = xmax;
			ymax = 50.0;
			ymin = plotdata->ymin = 0.0;
			plotdata->start = plot->group->start;
			plotdata->stop = plot->group->stop;
			plotdata->numbins = numbins;
			if (numbins > 0)
			{
				firstbinstart = plot->group->xrangemin;
				lastbinstop = plot->group->xrangemin + (numbins * bindur);
				if ((bins = (float *)calloc(numbins + 1, sizeof(float))) != NULL)
				{
					for (i=1; i <= plot->group->toe_nreps; i++)
					{
						data = plot->group->toe_repptrs[i];
						count = plot->group->toe_repcounts[i];
						while ((count >= 0) && (*data < firstbinstart)) { data++; count--; };
						while ((count >= 0) && (data[count-1] >= lastbinstop)) { count--; };
						for (j=0; j < count; j++)
						{
							bin = (int)floor(((data[j] - firstbinstart) / bindur));
							bins[bin] += 1.0;
						}
					}
					if (plotdata->normalize == TRUE)
					{
						for (j=0; j <= numbins; j++)
							bins[j] /= (plot->group->toe_nreps * plotdata->bindur) / 1000.0;
					}
				}
				for (ymax=0, i=0; i < numbins; i++)
					if (ymax < bins[i]) ymax = bins[i];
				ymax++;
				if ((!plotdata->ymax_override) && ((plotdata->ymax < ymax) || (plotdata->normalize == TRUE)))
				{
					plotdata->ymax = ymax;
				}
				else ymax = plotdata->ymax;
				if (ymax < 0) ymax = 50;
				plotdata->bins = bins;
			}
		}
		else
		{
			ymin = plotdata->ymin;
			ymax = plotdata->ymax;
			bins = plotdata->bins;
			numbins = plotdata->numbins;
			bindur = plotdata->bindur;
		}

		/*
		** Clear any marks
		*/
		plot->markx1 = -1;
		plot->markx2 = -1;

		/*
		** Do the rendering to an off-screen pixmap.
		*/
		DrawPSTH(dpy, plotdata->pixmap, plotdata->drawing_GC, offx, offy, width, height,
			bins, numbins, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			ymin, ymax, plotdata->boxes, plotdata->filled);

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

int psth_close(PLOT *plot)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
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

int psth_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
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
	** 3) add the extension: _psth_<entry>_<startms>_<stopms>.eps
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->filename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_psth_%ld_%d_%d.eps", plot->group->entry, (int)plot->group->xrangemin, (int)plot->group->xrangemax);
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
		PSDrawPSTH(fp, offx, offy, offx2, offy2, width, height,
			bins, numbins, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			ymin, ymax, plotdata->boxes, plotdata->filled);
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

int psth_save(PLOT *plot)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
	FILE *fp;
	char filenamestore[256], *filename, *ch, tempstr[40];
	float tmin, tmax, ttmp;
	int i;

	if ((plot->markx1 != -1) && (plot->markx2 != -1))
	{
		tmin = psth_conv_pixel_to_time(plot, plot->markx1);
		tmax = psth_conv_pixel_to_time(plot, plot->markx2);
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
	** 3) add the extension: _osc_<entry>_<startms>_<stopms>.pcm
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->filename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_psth_%ld_%d_%d.dat", plot->group->entry,
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
			printf("PSTH: saved %d histogram values into '%s'\n", plotdata->numbins, filename);
			fclose(fp);
		}
	}
	return SUCCESS;
}


static void psth_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
	XExposeEvent *expevent;
	int x, y0,y1;

	/*
	** Take care of the actual psth.  Then,
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
static void psth_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	plot->dirty = 1;
	psth_set(plot);
	return;
}


static void psth_clearmarks(PLOT *plot)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;

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

static void psth_drawstartmark(PLOT *plot, float t)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
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

static void psth_drawstopmark(PLOT *plot, float t)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
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


/*
**  Key press: ctrl-x, ctrl-y, etc.
**  Button1 press: evaluate at curpos
**  Button2 press: clear marks.  start mark at curpos.
**  Button2 motion: clear old stop mark.  stop mark at curpos
**  Button2 release: same as motion.
**  Button3 press: open the popup menu
*/
static void psth_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
{
	PLOT *plot = (PLOT *)clientData;
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
	XButtonEvent *butevent;
	XMotionEvent *motevent;
	XKeyEvent *keyevent;
	int x;
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
						defaults.psth_drawxaxis = plot->drawxaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "xaxis"), XmNset, plot->drawxaxis, NULL);
						psth_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'y'))
					{
						plot->dirty = 1;
						plot->drawyaxis = !(plot->drawyaxis);
						defaults.psth_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "yaxis"), XmNset, plot->drawyaxis, NULL);
						psth_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'n'))
					{
						plot->dirty = 1;
						plotdata->normalize = ! (plotdata->normalize);
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "normalize"), XmNset, plotdata->normalize, NULL);
						psth_set(plot);
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
						sprintf(title, "%.2f", psth_conv_pixel_to_time(plot, butevent->x));
						if (IS_Y_INBOUNDS(plot, butevent->y))
							sprintf(title + strlen(title), " (%.0f)", psth_conv_pixel_to_val(plot, butevent->y));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
					break;

				case Button2:
					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					/* Now draw and set the start mark */
					panel_drawstartmark(plot->panel, psth_conv_pixel_to_time(plot, butevent->x));

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", psth_conv_pixel_to_time(plot, butevent->x));
						sprintf(title + strlen(title), " : %.2f (0)", psth_conv_pixel_to_time(plot, butevent->x));
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
					panel_drawstopmark(plot->panel, psth_conv_pixel_to_time(plot, x));

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
				panel_drawstopmark(plot->panel, psth_conv_pixel_to_time(plot, x));

				if (IS_X_INBOUNDS(plot, motevent->x))
				{
					char title[256];
					float t1, t2;
					t1 = psth_conv_pixel_to_time(plot, MIN(plot->markx1, motevent->x));
					t2 = psth_conv_pixel_to_time(plot, MAX(plot->markx1, motevent->x));
					sprintf(title, "%.2f : %.2f (%.2f)", t1, t2, (t2 - t1));
					XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
				}
			}
			break;

		default:
			break;
	}
}

static void psth_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;
	char *idstring;
	Boolean drawxaxis, drawyaxis, value, drawboxes, donormalize, filled;
	int newymax, bindur, unit;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "X axis"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &drawxaxis, NULL);
		plot->drawxaxis = drawxaxis;
		defaults.psth_drawxaxis = plot->drawxaxis;
		psth_set(plot);
	}
	else if (!strcmp(idstring, "Y axis"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &drawyaxis, NULL);
		plot->drawyaxis = drawyaxis;
		defaults.psth_drawyaxis = plot->drawyaxis;
		psth_set(plot);
	}
	else if (!strncmp(idstring, "PSTH bindur", 8))
	{
		plot->dirty = 1;
		sscanf(idstring, "PSTH bindur %d", &bindur);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plot->group->psth_bindur != bindur))
		{
			plot->group->psth_bindur = bindur;
			defaults.psth_bindur = bindur;
			psth_display(plot);
		}
	}
	else if (!strncmp(idstring, "Ymax ", 5))
	{
#define YMAX_EQUALS_N(a) (ABS(plotdata->ymax - (a)) < 0.001)
		plot->dirty = 1;
		sscanf(idstring, "Ymax %d", &newymax);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && !(YMAX_EQUALS_N(newymax)))
		{
			plotdata->ymax_override = (newymax >= 0);
			plotdata->ymax = newymax;
			defaults.psth_ymax = newymax;
			psth_set(plot);
		}
#undef YMAX_EQUALS_N
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
			psth_display(plot);
		}
	}
	else if (!strcmp(idstring, "Filled"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &filled, NULL);
		plotdata->filled = filled;
		psth_set(plot);
	}
	else if (!strcmp(idstring, "Boxes"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &drawboxes, NULL);
		plotdata->boxes = drawboxes;
		psth_set(plot);
	}
	else if (!strcmp(idstring, "Normalize"))
	{
		plot->dirty = 1;
		XtVaGetValues(w, XmNset, &donormalize, NULL);
		plotdata->normalize = donormalize;
		psth_set(plot);
	}
	else if (!strcmp(idstring, "Save"))
	{
		psth_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		psth_print(plot, NULL, NULL);
	}
}

/*
** For the following, I employ the following notation:
** X, Y refer to pixel coordinates.
** TIME, VAL are in milliseconds and y units, respectively.
** SAMPLE is time in samples
*/
float psth_conv_pixel_to_time(PLOT *plot, int x)
{
	return ((((float)(x - plot->offx) / (float)(plot->width)) * 
			(float)(plot->group->xrangemax - plot->group->xrangemin)) + (float)plot->group->xrangemin);
}

float psth_conv_pixel_to_val(PLOT *plot, int y)
{
	struct psthdata *plotdata = (struct psthdata *)plot->plotdata;

	return (((float)plotdata->ymax - (((float)(y - plot->offy) / \
		(float)(plot->height)) * (float)(plotdata->ymax - plotdata->ymin))));
}


/*
** draws the psth in a pixmap
**
** dpy									== needed for XDrawSegments
** pixmap								== must be already created
** gc									== for drawing the line segments
** offx,offy,height,width				== defines a rectangle in which to draw
** samples, nsamples					== the signed short raw-data samples
** xmin,xmax,ymin,ymax					== the axis bounds
*/
static void DrawPSTH(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	float ymin, float ymax, boolean boxes, boolean filled)
{
	XPoint *pts;
	XSegment *segs;
	float pixels_per_bin, pixels_per_yrange, x0, x1;
	int i, nsegs, j;
	float binmin, binmax;
	XRectangle clipbox;

	if ((bins == NULL) || (numbins == 0) || (ymin >= ymax) || (xrangemin >= xrangemax) || (height < 10))
		return;

	clipbox.x = 0;
	clipbox.y = 0;
	clipbox.width = width;
	clipbox.height = height;
	XSetClipRectangles(dpy, gc, offx, offy, &clipbox, 1, Unsorted);

	pixels_per_yrange = ((float)height) / ((float)(ymax - ymin));
	pixels_per_bin = ((float)width) / ((float)(numbins));

	fprintf(stderr, "numbins = %d, width = %d\n", numbins, width);
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
				segs[i].y1 = offy + height /* - binmin * pixels_per_yrange */;
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

static void PSDrawPSTH(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float *bins, int numbins, int samplerate,
	float xrangemin, float xrangemax,
	float ymin, float ymax, boolean boxes, boolean filled)
{
	PSPoint *pts;
	PSSegment *segs;
	float pixels_per_yrange, pixels_per_bin, x0, x1;
	int i, nsegs, j, newwidth, newheight;
	float binmin, binmax;

	if ((bins == NULL) || (numbins == 0) || (ymin >= ymax) || (xrangemin >= xrangemax) || (height < 10))
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
#ifdef notdef
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
#else
			for (i=0; i < numbins; i++)
			{
				x0 = i * pixels_per_bin;
				x1 = (i+1) * pixels_per_bin;
				PSDrawFBoxFP(fp, x1 - x0, bins[i] * pixels_per_yrange, x0, 0.0);
			}
#endif
		}
	}
	fprintf(fp, "grestore\n");
	fprintf(fp, "closepath fill\n");
}

