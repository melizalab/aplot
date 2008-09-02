
/**********************************************************************************
 * INCLUDES
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
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
 * STRUCTURE DEFINITIONS
 **********************************************************************************/
typedef struct
{
	float time;
	char name[10];
} newlabel;

struct lbldata
{
	GC drawing_GC, inverse_GC, mark_GC;
	Pixmap pixmap;
	Boolean pixmapalloced;
	Boolean butgrabbed;
	XmFontList lblfont;

	int cycle;
	int n_uniq_lbls;
	int *lindex;
	int fontheight;
	int style;
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
#define CONV_MS_TO_X(plot, t) \
	(((((t) - plot->group->xrangemin) / (plot->group->xrangemax - plot->group->xrangemin)) * (plot->width)) + plot->offx)


#define CreateMenuToggle(widget, parent, name, label, set, userdata) \
	widget = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNset, (set), 															\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNvalueChangedCallback, label_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);

#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, label_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int label_open(PLOT *plot);
int label_display(PLOT *plot);
int label_set(PLOT *plot);
int label_close(PLOT *plot);
int label_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int label_save(PLOT *plot);
static void label_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void label_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void label_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void label_clearmarks(PLOT *plot);
static void label_drawstartmark(PLOT *plot, float t);
static void label_drawstopmark(PLOT *plot, float t);
static void label_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);
static void DrawLabels(Display *dpy, Pixmap pixmap, GC gc, XmFontList lblfont, int offx, int offy, int width, int height,
	int lblcount, float *lbltimes, char **lblnames, int *lindex, int n_uniq_lbls, int cycle, 
	float xrangemin, float xrangemax,
	int fontheight, int style);
static void PSDrawLabels(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	int lblcount, float *lbltimes, char **lblnames, int *lindex, int n_uniq_lbls, int cycle, 
	float xrangemin, float xrangemax,
	int fontheight, int style);
static void label_addlabels(PLOT *plot, newlabel *newlbls, int newlblcount);
static void label_dellabels(PLOT *plot, newlabel *newlbls, int newlblcount, float tmin, float tmax);
static int updatelblfile(char *filename, int entry, float *lbltimes, char **lblnames, int lblcount);

float label_conv_pixel_to_time(PLOT *plot, int x);
float label_conv_pixel_to_val(PLOT *plot, int y);

/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int label_open(PLOT *plot)
{
	struct lbldata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3, m4, m5, m6;
	Widget m6a, m6a1, m6a2, m6a3;
	int status = SUCCESS;
	Dimension width, height;
	int depth, offx, offy, offx2, offy2;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct lbldata));
	plotdata = (struct lbldata *)plot->plotdata;

	plotdata->cycle = -1;
	plotdata->n_uniq_lbls = 0;
	plotdata->lindex = NULL;
	plotdata->style = 2;
	plot->plot_widget = XtVaCreateManagedWidget("lbl", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.label_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);
	XtAddCallback(plot->plot_widget, XmNexposeCallback, label_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, label_resize_callback, (XtPointer)plot);

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
	plotdata->fontheight = XmStringHeight(plot->ticklblfont, xmstr = XmStringCreateSimple("1")); XmStringFree(xmstr);
	plot->minoffx = 6 + XmStringWidth(plot->ticklblfont, xmstr = XmStringCreateSimple("-32768")); XmStringFree(xmstr);
	plot->minoffx2 = 0;
	plot->minoffy = 0;
	plot->minoffy2 = 6 + plotdata->fontheight;

	plot->drawxaxis = defaults.label_drawxaxis;
	plot->drawyaxis = defaults.label_drawyaxis;

	offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
	offy = plot->offy = 0;
	offx2 = plot->offx2 = 0;
	offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
	plot->width = width - offx - offx2;
	plot->height = height - offy - offy2;

	plot->depth = depth;

	/*
	** Create the Graphics contexts.
	** drawing_GC is for the graph itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
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
	** Get the font we'll use for drawing the labels
	*/
	plotdata->lblfont = XmFontListCopy(_XmGetDefaultFontList(w, XmLABEL_FONTLIST));

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
	m6 = XtVaCreateManagedWidget("style", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Style"),
		XmNsubMenuId, m6a,
		NULL);
	CreateMenuToggle(m6a1, m6a, "style1", "Flat", (plotdata->style == 0), "Style 0");
	CreateMenuToggle(m6a2, m6a, "style2", "Cascade", (plotdata->style == 1), "Style 1");
	CreateMenuToggle(m6a3, m6a, "style3", "Bars", (plotdata->style == 2), "Style 2");

	/*
	** Register an event handler
	*/
	XtAddEventHandler(w, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
		FALSE, (XtEventHandler) label_event_handler, (XtPointer)plot);

	plot->plot_clearmarks = label_clearmarks;
	plot->plot_drawstartmark = label_drawstartmark;
	plot->plot_drawstopmark = label_drawstopmark;
	plot->plot_conv_pixel_to_time = label_conv_pixel_to_time;

	plot->plot_display = label_display;
	plot->plot_set = label_set;
	plot->plot_close = label_close;
	plot->plot_print = label_print;
	plot->plot_save = label_save;

	plot->plot_event_handler = label_event_handler;

	plotdata->butgrabbed = FALSE;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needlbl = 1;
	plot->dirty = 1;

	return status;
}

int label_display(PLOT *plot)
{
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
	int status = SUCCESS, tstatus;
	GROUP *group = plot->group;
	int i, j, l, *lindex;

	/*
	** Load the lbl data, but only if we need to...
	*/
	if ((plot->dirty == 1) || (group->lbldirty == 1))
	{
		group->samplerate = 20000;
		if (load_lbldata(&group->lblfp, group->loadedfilename, group->filename, group->entry, 
			&group->lblcount, &group->lbltimes, &group->lblnames, &group->start, &group->stop) == 0)
		{
			plot->dirty = 0;
			group->lbldirty = 0;
			if (group->xrangemin >= group->xrangemax)
			{
				group->xrangemin = group->start;
				group->xrangemax = group->stop;
			}

			/*
			** lindex is an array of indexes to unique labels.  There are n_uniq_lbls unique labels, which are
			** pointed to by the array indexes stored in lindex[0], lindex[1], ... lindex[n_uniq_lbls-1]
			** Compute these things because we might want to cycle through specific label types.  We might
			** even sort the indexes based on the alphabetic sorting of the labels, to make the order independent
			** of the order of occurence in this specific label file.
			*/
			if (plotdata->lindex != NULL)
			{
				free(plotdata->lindex);
				plotdata->lindex = NULL;
				plotdata->n_uniq_lbls = 0;
			}
			if ((lindex = (int *)malloc(group->lblcount * sizeof(int))) != NULL)
			{
				lindex[0] = 0;
				l = 1;
				for (i=1; i < group->lblcount; i++)
				{
					for (j=0; j < l; j++)
					{
						if (strcmp(group->lblnames[i], group->lblnames[lindex[j]]) == 0)
							break;
					}
					if (j == l) lindex[l++] = i;
				}
				plotdata->n_uniq_lbls = l;
				plotdata->lindex = lindex;
			}
		}
		else status = ERROR;
	}

	tstatus = label_set(plot);
	return (status == SUCCESS) ? (tstatus) : (status);
}

/*
** Before calling this function,
** you can:
** 		resize the window,				modify XmNwidth, XmNheight of widget
**		zoom/unzoom						modify plot->group->start,xrangemin,xrangemax
**		turn x/y axes on/off			modify plot->drawxaxis,drawyaxis
**		change the axis font			modify plot->ticklblfont,minoffx,minoffy,minoffx2,minoffy2
*/
int label_set(PLOT *plot)
{
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	int offx, offy, offx2, offy2;
	float *lbltimes;
	char **lblnames;
	int lblcount;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to label_open().
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
		defaults.label_height = height;
		defaults.width = width;

		/*
		** Set the axes margins (note: if you want to change
		** the ticklblfont, then you must also update the
		** plot->minoff?? numbers.  They are not recalculated
		** here.)  (see label_open)
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
		lblcount = plot->group->lblcount;
		lbltimes = plot->group->lbltimes;
		lblnames = plot->group->lblnames;

		/*
		** Clear any marks
		*/
		plot->markx1 = -1;
		plot->markx2 = -1;

		/*
		** Do the rendering to an off-screen pixmap.
		*/
		DrawLabels(dpy, plotdata->pixmap, plotdata->drawing_GC, plotdata->lblfont, offx, offy, width, height,
			lblcount, lbltimes, lblnames, plotdata->lindex, plotdata->n_uniq_lbls, plotdata->cycle, 
			plot->group->xrangemin, plot->group->xrangemax,
			plotdata->fontheight, plotdata->style);

		if (plot->drawxaxis == TRUE)
			DrawXAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
				offx, offy, width, height, plot->group->xrangemin, plot->group->xrangemax);

		if (plot->drawyaxis == TRUE)
			DrawYAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont, offx, offy, width, height, 0.0, 0.0);

		XCopyArea(dpy, plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
			0, 0, width + offx + offx2, height + offy + offy2, 0, 0);
	}
	return SUCCESS;
}

int label_close(PLOT *plot)
{
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
	Widget w = (Widget)plot->plot_widget;

	if (plotdata->lindex != NULL)
	{
		free(plotdata->lindex);
		plotdata->lindex = NULL;
		plotdata->n_uniq_lbls = 0;
	}

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

int label_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
	Dimension width, height;
	char filenamestore[256], *filename, *ch, tempstr[40];
	int offx, offy, offx2, offy2;
	int fileopenflag = 0;
	int cycle;
	int n_uniq_lbls;
	int *lindex;
	int lblcount;
	float *lbltimes;
	char **lblnames;

	/*
	** Create the filename of the output file:
	** 1) start with the input filename
	** 2) remove the extension
	** 3) add the extension: _lbl_<entry>_<startms>_<stopms>.eps
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->loadedfilename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_lbl_%ld_%d_%d.eps", plot->group->entry, (int)plot->group->xrangemin, (int)plot->group->xrangemax);
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
		lblcount = plot->group->lblcount;
		lbltimes = plot->group->lbltimes;
		lblnames = plot->group->lblnames;
		lindex = plotdata->lindex;
		n_uniq_lbls = plotdata->n_uniq_lbls;
		cycle = plotdata->cycle;

		PSDrawLabels(fp, offx, offy, offx2, offy2, width, height,
			lblcount, lbltimes, lblnames, lindex, n_uniq_lbls, cycle, 
			plot->group->xrangemin, plot->group->xrangemax,
			plotdata->fontheight, plotdata->style);
		if (plot->drawxaxis == TRUE)
			PSDrawXAxis(fp, offx, offy, offx2, offy2, width, height, plot->group->xrangemin, plot->group->xrangemax);
		if (plot->drawyaxis == TRUE)
			PSDrawYAxis(fp, offx, offy, offx2, offy2, width, height, 0, 0);

		PSFinish(fp);
		if (fileopenflag == 1)
			fclose(fp);
	}
	if (ret_filename_p) *ret_filename_p = strdup(filename);
	return SUCCESS;
}

int label_save(PLOT *plot)
{
	return SUCCESS;
}


static void label_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
	XExposeEvent *expevent;
	int x, y0,y1;

	/*
	** Take care of the actual graph.  Then,
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
static void label_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	label_set(plot);
	return;
}


/*
** DRAW_MARK draws a dashed vertical line, and is used when a range is being selected.
*/
#define DRAW_MARK(plot, plotdata, x) \
		{ \
			XDrawLine(XtDisplay(plot->plot_widget), XtWindow(plot->plot_widget), plotdata->mark_GC, \
				x, plot->offy, \
				x, plot->offy + plot->height); \
		}

static void label_clearmarks(PLOT *plot)
{
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
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

static void label_drawstartmark(PLOT *plot, float t)
{
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
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

static void label_drawstopmark(PLOT *plot, float t)
{
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
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

extern void change_file_entry(int direction);

/*
**  Key press: ctrl-x, ctrl-y, etc.
**  Button1 press: evaluate at curpos
**  Button2 press: clear marks.  start mark at curpos.
**  Button2 motion: clear old stop mark.  stop mark at curpos
**  Button2 release: same as motion.
**  Button3 press: open the popup menu
*/
static void label_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
{
	PLOT *plot = (PLOT *)clientData;
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
	XButtonEvent *butevent;
	XMotionEvent *motevent;
	XKeyEvent *keyevent;
	int x;
	char *keybuf;
	KeySym key_sym;
	Modifiers mod_return;
	Widget ew;
	float time, tmin, tmax, ttmp;
	newlabel nlbls[2];

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
						defaults.label_drawxaxis = plot->drawxaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "xaxis"), XmNset, plot->drawxaxis, NULL);
						label_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'y'))
					{
						plot->drawyaxis = !(plot->drawyaxis);
						defaults.label_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "yaxis"), XmNset, plot->drawyaxis, NULL);
						label_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (isdigit(keybuf[0])))
					{
						plotdata->cycle = keybuf[0] - '0' - 1;
						fprintf(stderr, "%d %d\n", plotdata->cycle, plotdata->n_uniq_lbls);
						if (plotdata->cycle >= plotdata->n_uniq_lbls)
							plotdata->cycle = -1;
						label_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (!strcmp(keybuf, "comma")))
					{
						change_file_entry(-1);
					}
					else if ((keyevent->state == ControlMask) && (!strcmp(keybuf, "period")))
					{
						change_file_entry(1);
					}
					else if (KEY_IS_LEFT(keybuf) || (KEY_IS_RIGHT(keybuf)) || (KEY_IS_UP(keybuf)) || (KEY_IS_DOWN(keybuf)))
					{
						panel_handle_keyrelease(w, plot, keyevent, keybuf);
					}
					else if (((keyevent->state & (ControlMask)) == 0) && (isalnum(keybuf[0])) && (keybuf[1] == '\0'))
					{
						if ((plot->markx1 != -1) && (plot->markx2 != -1))
						{
							tmin = label_conv_pixel_to_time(plot, plot->markx1);
							tmax = label_conv_pixel_to_time(plot, plot->markx2);
							if (tmin > tmax) { ttmp = tmax; tmax = tmin; tmin = ttmp; }
							nlbls[0].time = tmin;
							sprintf(nlbls[0].name, "%c-0", ((keyevent->state & ShiftMask) ? toupper(keybuf[0]) : keybuf[0]));
							nlbls[1].time = tmax;
							sprintf(nlbls[1].name, "%c-1", ((keyevent->state & ShiftMask) ? toupper(keybuf[0]) : keybuf[0]));
							label_addlabels(plot, nlbls, 2);
							panel_display(plot->panel);
						}
						else
						{
							if (IS_X_INBOUNDS(plot, keyevent->x))
							{
								time = label_conv_pixel_to_time(plot, keyevent->x);
								nlbls[0].time = time;
								sprintf(nlbls[0].name, "%c", ((keyevent->state & ShiftMask) ? toupper(keybuf[0]) : keybuf[0]));
								label_addlabels(plot, nlbls, 1);
								panel_display(plot->panel);
							}
						}
					}
					else if (((keyevent->state & (ControlMask|ShiftMask)) == 0) && ((KEY_IS_DELETE(keybuf)) || (KEY_IS_BACKSPACE(keybuf))))
					{
						if (IS_X_INBOUNDS(plot, keyevent->x))
						{
							time = label_conv_pixel_to_time(plot, keyevent->x);
							nlbls[0].time = time;
							*(nlbls[0].name) = '\0';
							label_dellabels(plot, nlbls, 1, plot->group->xrangemin, plot->group->xrangemax);
							panel_display(plot->panel);
						}
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
						sprintf(title, "%.2f", label_conv_pixel_to_time(plot, butevent->x));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
					break;

				case Button2:
					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					/* Now draw and set the start mark */
					panel_drawstartmark(plot->panel, label_conv_pixel_to_time(plot, butevent->x));

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", label_conv_pixel_to_time(plot, butevent->x));
						sprintf(title + strlen(title), " : %.2f (0)", label_conv_pixel_to_time(plot, butevent->x));
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
					panel_drawstopmark(plot->panel, label_conv_pixel_to_time(plot, x));

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
				panel_drawstopmark(plot->panel, label_conv_pixel_to_time(plot, x));

				if (IS_X_INBOUNDS(plot, motevent->x))
				{
					char title[256];
					float t1, t2;
					t1 = label_conv_pixel_to_time(plot, MIN(plot->markx1, motevent->x));
					t2 = label_conv_pixel_to_time(plot, MAX(plot->markx1, motevent->x));
					sprintf(title, "%.2f : %.2f (%.2f)", t1, t2, (t2 - t1));
					XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
				}
			}
			break;

		default:
			break;
	}
}

static void label_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	struct lbldata *plotdata = (struct lbldata *)plot->plotdata;
	char *idstring;
	Boolean drawxaxis, drawyaxis, value;
	int style = 0;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "X axis"))
	{
		XtVaGetValues(w, XmNset, &drawxaxis, NULL);
		plot->drawxaxis = drawxaxis;
		defaults.label_drawxaxis = plot->drawxaxis;
		label_set(plot);
	}
	else if (!strcmp(idstring, "Y axis"))
	{
		XtVaGetValues(w, XmNset, &drawyaxis, NULL);
		plot->drawyaxis = drawyaxis;
		defaults.label_drawyaxis = plot->drawyaxis;
		label_set(plot);
	}
	else if (!strncmp(idstring, "Style", 5))
	{
		sscanf(idstring, "Style %d", &style);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->style != style))
		{
			plotdata->style = style;
			label_set(plot);
		}
	}
	else if (!strcmp(idstring, "Save"))
	{
		label_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		label_print(plot, NULL, NULL);
	}
}

/*
** For the following, I employ the following notation:
** X, Y refer to pixel coordinates.
** TIME, VAL are in milliseconds and ? units, respectively.
** SAMPLE is time in samples
*/
float label_conv_pixel_to_time(PLOT *plot, int x)
{
	return ((((float)(x - plot->offx) / (float)(plot->width)) * 
		(float)(plot->group->xrangemax - plot->group->xrangemin)) + (float)plot->group->xrangemin);
}

float label_conv_pixel_to_val(PLOT *plot, int y)
{
	return 0.0;
}


/*
** draws the graph in a pixmap
**
** dpy									== needed for XDrawSegments
** pixmap								== must be already created
** gc									== for drawing the line segments
** offx,offy,height,width				== defines a rectangle in which to draw
** samples, nsamples					== the signed short raw-data samples
*/
static void DrawLabels(Display *dpy, Pixmap pixmap, GC gc, XmFontList lblfont, int offx, int offy, int width, int height,
	int lblcount, float *lbltimes, char **lblnames, int *lindex, int n_uniq_lbls, int cycle, 
	float xrangemin, float xrangemax,
	int fontheight, int style)
{
	XSegment *segs;
	XmString xmstr;
	float pixels_per_ms;
	int i, nsegs;
	float xstart, xstart1, xstart2;
	XRectangle clipbox;
	int numlbls, segidx;
	int nrows, rowlocs[10], prev_x2[10];
	int nrects, rectidx;
	XRectangle *rects;

	if ((lblcount == 0) || (lbltimes == NULL) || (lblnames == NULL) || (xrangemin >= xrangemax) || (height < 10))
		return;

	/* How many labels will there be? */
	numlbls = 0;
	for (i=0; i < lblcount; i++)
		if ((lbltimes[i] >= xrangemin) && (lbltimes[i] <= xrangemax))
			numlbls++;
	if (numlbls == 0)
		return;
	pixels_per_ms = ((float)width) / (xrangemax - xrangemin);

	clipbox.x = 0;
	clipbox.y = 0;
	clipbox.width = width;
	clipbox.height = height;
	XSetClipRectangles(dpy, gc, offx, offy, &clipbox, 1, Unsorted);

	if (style == 2)
	{
		nsegs = numlbls;
		if ((segs = (XSegment *)malloc(nsegs * sizeof(XSegment))) == NULL)
			goto finished;

		nrects = numlbls;
		if ((rects = (XRectangle *)malloc(nsegs * sizeof(XRectangle))) == NULL)
		{
			free(segs);
			segs = NULL;
			goto finished;
		}

		segidx = 0;
		rectidx = 0;
		for (i=0; i < lblcount; i++)
		{
			if ((cycle == -1) ||
				((lindex != NULL) && (n_uniq_lbls > 0) && (cycle < n_uniq_lbls) && (!strcmp(lblnames[i], lblnames[lindex[cycle]]))))
				{
					if ((lbltimes[i] >= xrangemin) && (lbltimes[i] <= xrangemax))
					{
						if ((strlen(lblnames[i]) >= 3) && (!strcmp(lblnames[i] + strlen(lblnames[i]) - 2, "-0")) &&
							(i < lblcount - 1) &&
							(lblnames[i][0] == lblnames[i+1][0]) &&
							(strlen(lblnames[i+1]) >= 3) && (!strcmp(lblnames[i+1] + strlen(lblnames[i+1]) - 2, "-1")) &&
							(lbltimes[i+1] >= xrangemin) && (lbltimes[i+1] <= xrangemax))
						{
							char *ch;

							xstart1 = (lbltimes[i] - xrangemin) * pixels_per_ms;
							segs[segidx].x1 = segs[segidx].x2 = (short) xstart1 + offx;
							segs[segidx].y1 = offy;
							segs[segidx].y2 = offy + height - 20;
							segidx++;

							xstart2 = (lbltimes[i+1] - xrangemin) * pixels_per_ms;
							segs[segidx].x1 = segs[segidx].x2 = (short) xstart2 + offx;
							segs[segidx].y1 = offy;
							segs[segidx].y2 = offy + height - 20;
							segidx++;

							rects[rectidx].x = xstart1 + offx;
							rects[rectidx].y = offy + height - 20 - 6;
							rects[rectidx].width = (int)(xstart2) - (int)(xstart1);
							rects[rectidx].height = 7;
							rectidx++;

							ch = strdup(lblnames[i]);
							ch[strlen(ch) - 2] = '\0';
							xmstr = XmStringCreateSimple(ch);
							xstart = 0.5 * (xstart1 + xstart2 - XmStringWidth(lblfont, xmstr));
							XmStringDraw(dpy, pixmap, lblfont, xmstr, gc, xstart + offx, offy + height - 19,
								100, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
							XmStringFree(xmstr);
							free(ch);
							i++;
						}
						else
						{
							xmstr = XmStringCreateSimple(lblnames[i]);
							xstart = (lbltimes[i] - xrangemin) * pixels_per_ms;
							segs[segidx].x1 = segs[segidx].x2 = (short) xstart + offx;
							segs[segidx].y1 = offy;
							segs[segidx].y2 = offy + height - 20;
							XmStringDraw(dpy, pixmap, lblfont, xmstr, gc, segs[segidx].x1, offy + height - 19,
								100, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
							segidx++;
							XmStringFree(xmstr);
						}
					}
				}
		}
		XDrawSegments(dpy, pixmap, gc, segs, segidx);
		XFillRectangles(dpy, pixmap, gc, rects, rectidx);
		free(rects);
		free(segs);
	}
	else
	{
		/*
		** Figure out how many rows will fit
		*/
		nrows = (int)floor((height - 20.0) / (fontheight + 1));
		if (nrows > 10) nrows = 10;
		for (i=0; i < nrows; i++)
		{
			rowlocs[i] = offy + height - 20 - (i * (fontheight + 1));
			prev_x2[i] = 0;
		}

		nsegs = numlbls;
		if ((segs = (XSegment *)malloc(nsegs * sizeof(XSegment))) != NULL)
		{
			int cur_row, prev_row;

			segidx = 0;
			cur_row = prev_row = 0;
			for (i=0; i < lblcount; i++)
			{
				if ((cycle == -1) ||
					((lindex != NULL) && (n_uniq_lbls > 0) && (cycle < n_uniq_lbls) && (!strcmp(lblnames[i], lblnames[lindex[cycle]]))))
					{
						if ((lbltimes[i] >= xrangemin) && (lbltimes[i] <= xrangemax))
						{
							xmstr = XmStringCreateSimple(lblnames[i]);
							xstart = (lbltimes[i] - xrangemin) * pixels_per_ms;

							/* If xstart < prev_x2, we have to increment the row */
							for (cur_row = 0; cur_row < nrows; cur_row++)
								if (xstart > prev_x2[cur_row])
									break;
							if (cur_row == nrows) cur_row = 0;
							if ((strlen(lblnames[i]) >= 3) && (!strcmp(lblnames[i] + strlen(lblnames[i]) - 2, "-1")))
								cur_row = prev_row;
							if (style == 0) cur_row = 0;

							segs[segidx].x1 = segs[segidx].x2 = (short) xstart + offx;
							segs[segidx].y1 = offy;
							segs[segidx].y2 = rowlocs[cur_row];

							XmStringDraw(dpy, pixmap, lblfont, xmstr, gc, segs[segidx].x1, rowlocs[cur_row] + 1,
								100, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
							prev_x2[cur_row] = xstart + 1 + XmStringWidth(lblfont, xmstr);
							prev_row = cur_row;
							XmStringFree(xmstr);
							segidx++;
						}
					}
			}
			XDrawSegments(dpy, pixmap, gc, segs, segidx);
			free(segs);
		}
	}

finished:
	XSetClipMask(dpy, gc, None);
}

#define PRINTER_DPI 600.0

static void PSDrawLabels(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	int lblcount, float *lbltimes, char **lblnames, int *lindex, int n_uniq_lbls, int cycle, 
	float xrangemin, float xrangemax,
	int fontheight, int style)
{
	PSSegment *segs;
	float pixels_per_ms;
	int i, nsegs, newwidth, newheight;
	int numlbls, segidx;
	float xstart, xstart1, xstart2;
	int nrows;
	float rowlocs[10], prev_x2[10];
	int nrects, rectidx;
	PSRectangle *rects;

	if ((lblcount == 0) || (lbltimes == NULL) || (lblnames == NULL) || (xrangemin >= xrangemax) || (height < 10))
		return;

	/* How many labels will there be? */
	numlbls = 0;
	for (i=0; i < lblcount; i++)
		if ((lbltimes[i] >= xrangemin) && (lbltimes[i] <= xrangemax))
			numlbls++;
	if (numlbls == 0)
		return;

	newwidth = width * PRINTER_DPI / 72.0;
	newheight = height * PRINTER_DPI / 72.0;

	fprintf(fp, "gsave\n");
	fprintf(fp, "%d %d translate\n", offx, offy2);
	fprintf(fp, "%f %f scale\n", 72.0 / PRINTER_DPI, 72.0 / PRINTER_DPI);

	pixels_per_ms = ((float)newwidth) / (xrangemax - xrangemin);

	fprintf(fp, "newpath\n");
	fprintf(fp, "0.00 0.00 0.00 setrgbcolor /Times-Roman findfont %04d scalefont setfont\n", (int)(10 * PRINTER_DPI / 72));

	if (style == 0)
	{
		nsegs = numlbls;
		if ((segs = (PSSegment *)malloc(nsegs * sizeof(PSSegment))) != NULL)
		{
			segidx = 0;
			for (i=0; i < lblcount; i++)
			{
				if ((cycle == -1) ||
					((lindex != NULL) && (n_uniq_lbls > 0) && (cycle < n_uniq_lbls) && (!strcmp(lblnames[i], lblnames[lindex[cycle]]))))
					{
						if ((lbltimes[i] >= xrangemin) && (lbltimes[i] <= xrangemax))
						{
							xstart = (lbltimes[i] - xrangemin) * pixels_per_ms;
							segs[segidx].x1 = segs[segidx].x2 = xstart;
							segs[segidx].y1 = newheight;
							segs[segidx].y2 = (20 * newheight * 1.0 / height) + (10 * PRINTER_DPI / 72);
							fprintf(fp, "%f %f moveto (%s) show\n", segs[segidx].x1, segs[segidx].y2 - (10 * PRINTER_DPI / 72), lblnames[i]);
							segidx++;
						}
					}
			}
			fprintf(fp, "newpath\n");
			PSDrawSegmentsFP(fp, segs, nsegs);
			free(segs);
		}
	}
	else if (style == 1)
	{
		/* Figure out how many rows will fit */
		nrows = (int)floor((height - 20.0) / (fontheight + 1));
		if (nrows > 10) nrows = 10;
		fontheight = 10 * PRINTER_DPI / 72;
		for (i=0; i < nrows; i++)
		{
			rowlocs[i] = (20 * newheight * 1.0 / height) + ((i + 1) * (fontheight + 1));
			prev_x2[i] = 0;
		}

		nsegs = numlbls;
		if ((segs = (PSSegment *)malloc(nsegs * sizeof(PSSegment))) != NULL)
		{
			int cur_row, prev_row;

			segidx = 0;
			cur_row = prev_row = 0;
			for (i=0; i < lblcount; i++)
			{
				if ((cycle == -1) ||
					((lindex != NULL) && (n_uniq_lbls > 0) && (cycle < n_uniq_lbls) && (!strcmp(lblnames[i], lblnames[lindex[cycle]]))))
					{
						if ((lbltimes[i] >= xrangemin) && (lbltimes[i] <= xrangemax))
						{
							xstart = (lbltimes[i] - xrangemin) * pixels_per_ms;

							/* If xstart < prev_x2, we have to increment the row */
							for (cur_row = 0; cur_row < nrows; cur_row++)
								if (xstart > prev_x2[cur_row])
									break;
							if (cur_row == nrows) cur_row = 0;
							if ((strlen(lblnames[i]) >= 3) && (!strcmp(lblnames[i] + strlen(lblnames[i]) - 2, "-1")))
								cur_row = prev_row;

							segs[segidx].x1 = segs[segidx].x2 = xstart;
							segs[segidx].y1 = newheight;
							segs[segidx].y2 = rowlocs[cur_row];
							fprintf(fp, "%f %f moveto (%s) show\n",
								xstart,
								rowlocs[cur_row] + 1,
								lblnames[i]);
							{
								/*** HACK *** HACK *** HACK ***/
								prev_x2[cur_row] = xstart + 1 + (strlen(lblnames[i]) * 10 * PRINTER_DPI / 72);
								/*** HACK *** HACK *** HACK ***/
							}
							prev_row = cur_row;
							segidx++;
						}
					}
			}
			fprintf(fp, "newpath\n");
			PSDrawSegmentsFP(fp, segs, nsegs);
			free(segs);
		}
	}
	else if (style == 2)
	{
		nsegs = numlbls;
		if ((segs = (PSSegment *)malloc(nsegs * sizeof(PSSegment))) != NULL)
		{
			nrects = numlbls;
			if ((rects = (PSRectangle *)malloc(nrects * sizeof(PSRectangle))) != NULL)
			{
				segidx = 0;
				rectidx = 0;
				for (i=0; i < lblcount; i++)
				{
					if ((cycle == -1) ||
						((lindex != NULL) && (n_uniq_lbls > 0) && (cycle < n_uniq_lbls) && (!strcmp(lblnames[i], lblnames[lindex[cycle]]))))
						{
							if ((lbltimes[i] >= xrangemin) && (lbltimes[i] <= xrangemax))
							{
								if ((strlen(lblnames[i]) >= 3) && (!strcmp(lblnames[i] + strlen(lblnames[i]) - 2, "-0")) &&
									(i < lblcount - 1) &&
									(lblnames[i][0] == lblnames[i+1][0]) &&
									(strlen(lblnames[i+1]) >= 3) && (!strcmp(lblnames[i+1] + strlen(lblnames[i+1]) - 2, "-1")) &&
									(lbltimes[i+1] >= xrangemin) && (lbltimes[i+1] <= xrangemax))
								{
									char *ch;

									xstart1 = (lbltimes[i] - xrangemin) * pixels_per_ms;
									segs[segidx].x1 = segs[segidx].x2 = xstart1;
									segs[segidx].y1 = newheight;
									segs[segidx].y2 = (20 * newheight * 1.0 / height) + (10 * PRINTER_DPI / 72);
									segidx++;

									xstart2 = (lbltimes[i+1] - xrangemin) * pixels_per_ms;
									segs[segidx].x1 = segs[segidx].x2 = xstart2;
									segs[segidx].y1 = newheight;
									segs[segidx].y2 = (20 * newheight * 1.0 / height) + (10 * PRINTER_DPI / 72);
									segidx++;

									rects[rectidx].x = xstart1;
									rects[rectidx].y = (20 * newheight * 1.0 / height) + (10 * PRINTER_DPI / 72);
									rects[rectidx].width = xstart2 - xstart1;
									rects[rectidx].height = 5 * (PRINTER_DPI / 72);
									rectidx++;

									ch = strdup(lblnames[i]);
									ch[strlen(ch) - 2] = '\0';
									fprintf(fp, "%f %f moveto (%s) stringwidth pop -0.5 mul 0 rmoveto (%s) show\n",
										0.5 * (xstart1 + xstart2),
										segs[segidx - 1].y2 - (10 * PRINTER_DPI / 72),
										ch, ch);
									free(ch);
									i++;
								}
								else
								{
									xstart = (lbltimes[i] - xrangemin) * pixels_per_ms;
									segs[segidx].x1 = segs[segidx].x2 = xstart;
									segs[segidx].y1 = newheight;
									segs[segidx].y2 = (20 * newheight * 1.0 / height) + (10 * PRINTER_DPI / 72);
									fprintf(fp, "%f %f moveto (%s) show\n", segs[segidx].x1, segs[segidx].y2 - (10 * PRINTER_DPI / 72), lblnames[i]);
									segidx++;
								}
							}
						}
				}
				fprintf(fp, "newpath\n");
				PSDrawSegmentsFP(fp, segs, segidx);
				PSFillRectangles(fp, rects, rectidx);
				free(rects);
			}
			free(segs);
		}
	}

	fprintf(fp, "grestore\n");
	fprintf(fp, "closepath fill\n");
}



static void label_addlabels(PLOT *plot, newlabel *newlbls, int newlblcount)
{
	float *lbltimes, *newlbltimes;
	char **lblnames, **newlblnames;
	int lblcount, i, j, k;

	lblcount = plot->group->lblcount;
	lbltimes = plot->group->lbltimes;
	lblnames = plot->group->lblnames;

	newlblnames = (char **)calloc(lblcount + newlblcount, sizeof(char *));
	newlbltimes = (float *)calloc(lblcount + newlblcount, sizeof(float));

	/*
	** Copy the old labels into the new labels, adding the ones passed in the newlbls[] array
	*/
	for (i=0, j=0, k=0; i < lblcount + newlblcount; i++)
	{
		if ((j < lblcount) && ((k == newlblcount) || (lbltimes[j] < newlbls[k].time)))
		{
			newlbltimes[i] = lbltimes[j];
			newlblnames[i] = lblnames[j];
			j++;
		}
		else
		{
			newlbltimes[i] = newlbls[k].time;
			newlblnames[i] = newlbls[k].name;
			k++;
		}
	}

	if (updatelblfile(plot->group->loadedfilename, plot->group->entry, newlbltimes, newlblnames, lblcount + newlblcount) == -1)
		fprintf(stderr, "WARNING: error occurred while trying to update label file...\n");

	free(newlblnames);
	free(newlbltimes);
	plot->dirty = 1;
	plot->group->lbldirty = 1;
}

static void label_dellabels(PLOT *plot, newlabel *newlbls, int newlblcount, float tmin, float tmax)
{
	float *lbltimes, *newlbltimes;
	char **lblnames, **newlblnames;
	int lblcount, i, j, k, *lbldelete;
	float tleft, tright;

	lblcount = plot->group->lblcount;
	lbltimes = plot->group->lbltimes;
	lblnames = plot->group->lblnames;

	if (lblcount == 0)
		return;

	newlblnames = (char **)calloc(lblcount, sizeof(char *));
	newlbltimes = (float *)calloc(lblcount, sizeof(float));
	lbldelete = (int *)calloc(lblcount, sizeof(int));

	/*
	** Find the label in lbltimes[] that is closest to newlbls[i]
	*/
	for (k=0; k < newlblcount; k++)
	{
		if (newlbls[k].time < lbltimes[0])
		{
			lbldelete[0] = 1;
			continue;
		}
		if (newlbls[k].time > lbltimes[lblcount-1])
		{
			lbldelete[lblcount-1] = 1;
			continue;
		}
		for (i=0; i < lblcount-1; i++)
		{
			if ((newlbls[k].time >= lbltimes[i]) && (newlbls[k].time <= lbltimes[i+1]))
			{
				tleft = newlbls[k].time - lbltimes[i];
				tright = lbltimes[i+1] - newlbls[k].time;
				if (tleft <= tright)
					lbldelete[i] = 1;
				else
					lbldelete[i+1] = 1;
				continue;
			}
		}
	}

	/*
	** Only delete labels that are visible!
	*/
	for (i=0; i < lblcount; i++)
		if ((lbldelete[i] == 1) && ((lbltimes[i] < tmin) || (lbltimes[i] > tmax)))
			lbldelete[i] = 0;

	/*
	** Copy the labels that aren't marked for deletion
	*/
	for (i=0, j=0; i < lblcount; i++)
	{
		if (lbldelete[i] != 1)
		{
			newlbltimes[j] = lbltimes[i];
			newlblnames[j] = lblnames[i];
			j++;
		}
	}
	newlblcount = j;


	if (updatelblfile(plot->group->loadedfilename, plot->group->entry, newlbltimes, newlblnames, newlblcount) == -1)
		fprintf(stderr, "WARNING: error occurred while trying to update label file...\n");

	free(lbldelete);
	free(newlblnames);
	free(newlbltimes);
	plot->dirty = 1;
	plot->group->lbldirty = 1;
}

static int updatelblfile(char *filename, int entry, float *lbltimes, char **lblnames, int lblcount)
{
	LBLFILE *lblfp;
	PCMFILE *pcmfp;
	char filenamebuf[256], *ch;
	int rc, caps;

	/*
	** Write out the new label file
	** It is possible that the filename in group->filename actually refers to a PCM file (that
	** we are labelling.)  Therefore, if lbl_open() returns an error, then try pcm_open to see
	** if it's a PCM file, and if it is, then create a new filename based on the PCM filename
	** and entry, and use this file.  This code must be the same as in label_dellabels() and
	** panel_display().
	*/
	if ((lblfp = lbl_open(filename, "w")) == NULL)
	{
		/* Okay, filename isn't a LBL file.  Check if it's a PCM file */
		rc = -1;
		if ((pcmfp = pcm_open(filename, "r")) != NULL)
		{
			if (pcm_seek(pcmfp, entry) != -1)
			{
				strcpy(filenamebuf, filename);
				ch = strrchr(filenamebuf, '.');
				if ((pcm_ctl(pcmfp, PCMIOGETCAPS, &caps) != -1) && ((caps & PCMIOCAP_MULTENTRY) == 0))
					sprintf(ch, ".lbl");
				else
					sprintf(ch, "_%03d.lbl", entry);
				if ((lblfp = lbl_open(filenamebuf, "w")) != NULL)
				{
					if (lbl_write(lblfp, lbltimes, lblnames, lblcount) == -1)
						fprintf(stderr, "ERROR: Couldn't write to label file '%s'\n", filename);
					else rc = 0;
					lbl_close(lblfp);
				}
				else fprintf(stderr, "Couldn't write to %s\n", filenamebuf);
			}
			else fprintf(stderr, "Couldn't open entry %d of %s\n", entry, filename);
			pcm_close(pcmfp);
		}
		else fprintf(stderr, "Couldn't open %s\n", filename);
	}
	else
	{
		rc = 0;
		if (lbl_write(lblfp, lbltimes, lblnames, lblcount) == -1)
		{
			fprintf(stderr, "ERROR: Couldn't write to label file '%s'\n", filename);
			rc = -1;
		}
		lbl_close(lblfp);
	}
	return rc;
}

