
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
struct rastdata
{
	GC drawing_GC, inverse_GC, mark_GC;
	Pixmap pixmap;
	Boolean pixmapalloced;
	Boolean butgrabbed;
	XmFontList lblfont;
	int xmin,xmax,ymin,ymax;
	int dotshape;
	int autoysize;
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
	XtAddCallback(widget, XmNvalueChangedCallback, rast_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);

#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, rast_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int rast_open(PLOT *plot);
int rast_display(PLOT *plot);
int rast_set(PLOT *plot);
int rast_close(PLOT *plot);
int rast_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int rast_save(PLOT *plot);
static void rast_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void rast_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void rast_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void rast_clearmarks(PLOT *plot);
static void rast_drawstartmark(PLOT *plot, float t);
static void rast_drawstopmark(PLOT *plot, float t);
static void calc_statistics(PLOT *plot, float tmin, float tmax);
static void rast_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);
static void DrawRAST(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float **repptrs, int *repcounts, int nreps, int samplerate,
	float xrangemin, float xrangemax,
	int ymin, int ymax, int dotshape);
static void PSDrawRAST(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float **repptrs, int *repcounts, int nreps, int samplerate,
	float xrangemin, float xrangemax,
	int ymin, int ymax, int dotshape);

float rast_conv_pixel_to_time(PLOT *plot, int x);
float rast_conv_pixel_to_val(PLOT *plot, int y);


/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int rast_open(PLOT *plot)
{
	struct rastdata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3, m4, m5, m6, m7, m8;
	Widget m6a, m6a1, m6a2, m6a3, m6a4, m6a5, m6a6, m6a7, m6a8;
	Widget m8a, m8a1, m8a2, m8a3, m8a4, m8a5, m8a6, m8a7, m8a8, m8a9, m8a10;
	int status = SUCCESS;
	Dimension width, height;
	int depth, offx, offy, offx2, offy2;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct rastdata));
	plotdata = (struct rastdata *)plot->plotdata;

	plotdata->autoysize = defaults.rast_autoysize;
	plotdata->dotshape = defaults.rast_dotshape;

	plot->plot_widget = XtVaCreateManagedWidget("rast", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.rast_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);
	XtAddCallback(plot->plot_widget, XmNexposeCallback, rast_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, rast_resize_callback, (XtPointer)plot);

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

	plot->drawxaxis = defaults.rast_drawxaxis;
	plot->drawyaxis = defaults.rast_drawyaxis;

	offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
	offy = plot->offy = 0;
	offx2 = plot->offx2 = 0;
	offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
	plot->width = width - offx - offx2;
	plot->height = height - offy - offy2;

	plot->depth = depth;

	/*
	** Create the Graphics contexts.
	** drawing_GC is for the rast itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
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

	CreateMenuToggle(m7, mw, "autoysize", "AutoYSize", plotdata->autoysize, "AutoYSize");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m8a = XmCreatePulldownMenu(mw, "m8a", args, n);
	m8 = XtVaCreateManagedWidget("dotshape", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("DotShape"),
		XmNsubMenuId, m8a,
		NULL);
	CreateMenuToggle(m8a1, m8a, "dotshape1", "1, 0", (plotdata->dotshape == 1), "DotShape 1");
	CreateMenuToggle(m8a2, m8a, "dotshape2", "1, 1", (plotdata->dotshape == 2), "DotShape 2");
	CreateMenuToggle(m8a3, m8a, "dotshape3", "2, 0", (plotdata->dotshape == 3), "DotShape 3");
	CreateMenuToggle(m8a4, m8a, "dotshape4", "2, 1", (plotdata->dotshape == 4), "DotShape 4");
	CreateMenuToggle(m8a5, m8a, "dotshape5", "3, 0", (plotdata->dotshape == 5), "DotShape 5");
	CreateMenuToggle(m8a6, m8a, "dotshape6", "3, 1", (plotdata->dotshape == 6), "DotShape 6");
	CreateMenuToggle(m8a7, m8a, "dotshape7", "4, 0", (plotdata->dotshape == 7), "DotShape 7");
	CreateMenuToggle(m8a8, m8a, "dotshape8", "4, 1", (plotdata->dotshape == 8), "DotShape 8");
	CreateMenuToggle(m8a9, m8a, "dotshape9", "8, 0", (plotdata->dotshape == 9), "DotShape 9");
	CreateMenuToggle(m8a10,m8a, "dotshape10","8, 1", (plotdata->dotshape == 10),"DotShape 10");

	/*
	** Register an event handler
	*/
	XtAddEventHandler(w, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
		FALSE, (XtEventHandler) rast_event_handler, (XtPointer)plot);

	plot->plot_clearmarks = rast_clearmarks;
	plot->plot_drawstartmark = rast_drawstartmark;
	plot->plot_drawstopmark = rast_drawstopmark;
	plot->plot_conv_pixel_to_time = rast_conv_pixel_to_time;

	plot->plot_display = rast_display;
	plot->plot_set = rast_set;
	plot->plot_close = rast_close;
	plot->plot_print = rast_print;
	plot->plot_save = rast_save;

	plot->plot_event_handler = rast_event_handler;

	plotdata->butgrabbed = FALSE;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needtoe = 1;

	return status;
}

int rast_display(PLOT *plot)
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

	tstatus = rast_set(plot);
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
int rast_set(PLOT *plot)
{
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	int xmin, xmax, ymin, ymax, offx, offy, offx2, offy2;
	float **repptrs;
	int *repcounts, nreps;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to rast_open().
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

		defaults.rast_height = height;
		defaults.width = width;

		/*
		** Set the axes margins (note: if you want to change
		** the ticklblfont, then you must also update the
		** plot->minoff?? numbers.  They are not recalculated
		** here.)  (see rast_open)
		*/
		offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
		offy = plot->offy = 0;
		offx2 = plot->offx2 = 0;
		offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);

		if ((plotdata->autoysize == TRUE) && (XtIsManaged(w)))
		{
			int rowheight, rowspacing;
			Dimension newheight;

			switch (plotdata->dotshape)
			{
				case 1: rowheight = 1; rowspacing = 0; break;
				case 2: rowheight = 1; rowspacing = 1; break;
				case 3: rowheight = 2; rowspacing = 0; break;
				case 4: rowheight = 2; rowspacing = 1; break;
				case 5: rowheight = 3; rowspacing = 0; break;
				case 6: rowheight = 3; rowspacing = 1; break;
				case 7: rowheight = 4; rowspacing = 0; break;
				case 8: rowheight = 4; rowspacing = 1; break;
				case 9: rowheight = 8; rowspacing = 0; break;
				case 10: rowheight = 8; rowspacing = 1; break;
				default: rowheight = 1; rowspacing = 0; break;
			}

			newheight = plot->offy + plot->offy2 + ((plot->group->toe_nreps + 1) * (rowheight + rowspacing));
			if (newheight != height)
			{
				XtUnmanageChild(w);
				XtVaSetValues(w,
					XmNheight, newheight,
					XmNwidth, width,
					NULL);
				XtVaSetValues(plot->panel->panel_container,
					XmNwidth, width,
					NULL);
				XtManageChild(w);
				XtVaGetValues(w,
					XmNheight, &height,
					XmNwidth, &width,
					NULL);
			}
		}

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
		ymin = 0;
		ymax = plot->group->toe_nreps + 1;
		plotdata->xmin = xmin;
		plotdata->xmax = xmax;
		plotdata->ymin = ymin;
		plotdata->ymax = ymax;

		nreps = plot->group->toe_nreps;
		repptrs = plot->group->toe_repptrs;
		repcounts = plot->group->toe_repcounts;

		/*
		** Clear any marks
		*/
		plot->markx1 = -1;
		plot->markx2 = -1;

		/*
		** Do the rendering to an off-screen pixmap.
		*/
		DrawRAST(dpy, plotdata->pixmap, plotdata->drawing_GC, offx, offy, width, height,
			repptrs, repcounts, nreps, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			ymin, ymax, plotdata->dotshape);

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

int rast_close(PLOT *plot)
{
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;
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

int rast_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;
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
	** 3) add the extension: _rast_<entry>_<startms>_<stopms>.eps
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->filename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	snprintf(tempstr, sizeof(tempstr), "_rast_%ld_%d_%d.eps", plot->group->entry,
		(int)plot->group->xrangemin, (int)plot->group->xrangemax);
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

		PSDrawRAST(fp, offx, offy, offx2, offy2, width, height,
			repptrs, repcounts, nreps, plot->group->samplerate,
			plot->group->xrangemin, plot->group->xrangemax,
			ymin, ymax, plotdata->dotshape);
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

int rast_save(PLOT *plot)
{
	return SUCCESS;
}


static void rast_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;
	XExposeEvent *expevent;
	int x, y0,y1;

	/*
	** Take care of the actual rast.  Then,
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
static void rast_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	rast_set(plot);
	return;
}

static void rast_clearmarks(PLOT *plot)
{
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;

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

static void rast_drawstartmark(PLOT *plot, float t)
{
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;
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

static void rast_drawstopmark(PLOT *plot, float t)
{
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;
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
static void rast_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
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
						defaults.rast_drawxaxis = plot->drawxaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "xaxis"), XmNset, plot->drawxaxis, NULL);
						rast_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'y'))
					{
						plot->drawyaxis = !(plot->drawyaxis);
						defaults.rast_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "yaxis"), XmNset, plot->drawyaxis, NULL);
						rast_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 's'))
					{
						if ((plot->markx1 != -1) && (plot->markx2 != -1))
						{
							tmin = rast_conv_pixel_to_time(plot, plot->markx1);
							tmax = rast_conv_pixel_to_time(plot, plot->markx2);
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
						sprintf(title, "%.2f", rast_conv_pixel_to_time(plot, butevent->x));
						if (IS_Y_INBOUNDS(plot, butevent->y))
							sprintf(title + strlen(title), " (%.0f)", rast_conv_pixel_to_val(plot,  butevent->y));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
					break;

				case Button2:
					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					/* Now draw and set the start mark */
					panel_drawstartmark(plot->panel, rast_conv_pixel_to_time(plot, butevent->x));

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", rast_conv_pixel_to_time(plot, butevent->x));
						sprintf(title + strlen(title), " : %.2f (0)", rast_conv_pixel_to_time(plot, butevent->x));
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
					panel_drawstopmark(plot->panel, rast_conv_pixel_to_time(plot, x));

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
				panel_drawstopmark(plot->panel, rast_conv_pixel_to_time(plot, x));

				if (IS_X_INBOUNDS(plot, motevent->x))
				{
					char title[256];
					float t1, t2;
					t1 = rast_conv_pixel_to_time(plot, MIN(plot->markx1, motevent->x));
					t2 = rast_conv_pixel_to_time(plot, MAX(plot->markx1, motevent->x));
					sprintf(title, "%.2f : %.2f (%.2f)", t1, t2, (t2 - t1));
					XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
				}
			}
			break;

		default:
			break;
	}
}

static void rast_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;
	char *idstring;
	Boolean drawxaxis, drawyaxis, value, autoysize;
	int unit, dotshape;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "X axis"))
	{
		XtVaGetValues(w, XmNset, &drawxaxis, NULL);
		plot->drawxaxis = drawxaxis;
		defaults.rast_drawxaxis = plot->drawxaxis;
		rast_set(plot);
	}
	else if (!strcmp(idstring, "Y axis"))
	{
		XtVaGetValues(w, XmNset, &drawyaxis, NULL);
		plot->drawyaxis = drawyaxis;
		defaults.rast_drawyaxis = plot->drawyaxis;
		rast_set(plot);
	}
	else if (!strncmp(idstring, "DotShape", 8))
	{
		sscanf(idstring, "DotShape %d", &dotshape);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->dotshape != dotshape))
		{
			plotdata->dotshape = dotshape;
			defaults.rast_dotshape = dotshape;
			rast_display(plot);
		}
	}
	else if (!strncmp(idstring, "Unit", 4))
	{
		plot->group->toedirty = 1;
		sscanf(idstring, "Unit %d", &unit);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plot->group->toe_curunit != unit))
		{
			plot->group->toe_curunit = unit;
			rast_display(plot);
		}
	}
	else if (!strcmp(idstring, "Save"))
	{
		rast_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		rast_print(plot, NULL, NULL);
	}
	else if (!strcmp(idstring, "AutoYSize"))
	{
		XtVaGetValues(w, XmNset, &autoysize, NULL);
		plotdata->autoysize = autoysize;
		defaults.rast_autoysize = autoysize;
		rast_set(plot);
	}
}


/*
** For the following, I employ the following notation:
** X, Y refer to pixel coordinates.
** TIME, VAL are in milliseconds and y units, respectively.
** SAMPLE is time in samples
*/
float rast_conv_pixel_to_time(PLOT *plot, int x)
{
	return ((((float)(x - plot->offx) / (float)(plot->width)) * 
		(float)(plot->group->xrangemax - plot->group->xrangemin)) + (float)plot->group->xrangemin);
}

float rast_conv_pixel_to_val(PLOT *plot, int y)
{
	struct rastdata *plotdata = (struct rastdata *)plot->plotdata;

	return (((float)plotdata->ymax - (((float)(y - plot->offy) / \
		(float)(plot->height)) * (float)(plotdata->ymax - plotdata->ymin))));
}


/*
** draws the rast in a pixmap
**
** dpy									== needed for XDrawSegments
** pixmap								== must be already created
** gc									== for drawing the line segments
** offx,offy,height,width				== defines a rectangle in which to draw
** samples, nsamples					== the signed short raw-data samples
** xrangemin, xrangemax                 == the axis bounds (in ms)
** ymin,ymax					== the axis bounds
*/
static void DrawRAST(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	float **repptrs, int *repcounts, int nreps, int samplerate,
	float xrangemin, float xrangemax,
	int ymin, int ymax, int dotshape)
{
	XPoint *pts;
	float pixels_per_ms, pixels_per_row, winstart, winstop;
	int i, nsegs, y, rep;
	XRectangle clipbox;
	float *data;
	int count, start, stop, prevx;
	int rowheight, rowspacing, h;

	if ((repptrs == NULL) || (repcounts == NULL) || (nreps == 0) || (ymin >= ymax) || (xrangemin >= xrangemax) || (height < 1))
		return;

	clipbox.x = 0;
	clipbox.y = 0;
	clipbox.width = width;
	clipbox.height = height;
	XSetClipRectangles(dpy, gc, offx, offy, &clipbox, 1, Unsorted);

	switch (dotshape)
	{
		case 1: rowheight = 1; rowspacing = 0; break;
		case 2: rowheight = 1; rowspacing = 1; break;
		case 3: rowheight = 2; rowspacing = 0; break;
		case 4: rowheight = 2; rowspacing = 1; break;
		case 5: rowheight = 3; rowspacing = 0; break;
		case 6: rowheight = 3; rowspacing = 1; break;
		case 7: rowheight = 4; rowspacing = 0; break;
		case 8: rowheight = 4; rowspacing = 1; break;
		case 9: rowheight = 8; rowspacing = 0; break;
		case 10: rowheight = 8; rowspacing = 1; break;
		default: rowheight = 1; rowspacing = 0; break;
	}

	winstart = xrangemin;
	winstop = xrangemax;
	pixels_per_ms = ((float)width) / (xrangemax - xrangemin);
	pixels_per_row = ((float)height) / ((float)(ymax - ymin));

	if (pixels_per_row > (rowheight + rowspacing))
		pixels_per_row = rowheight + rowspacing;

	for (rep = 1; rep <= nreps; rep++)
	{
		data = repptrs[rep];
		count = repcounts[rep];
		start = 0;
		while ((start < count) && (data[start] < winstart)) start++;
		stop = start;
		while ((stop < count) && (data[stop] < winstop)) stop++;
		count = stop - start;
		if (count > 0)
		{
			y = offy + (rep) * pixels_per_row;

			nsegs = count;
			if ((pts = (XPoint *)malloc(nsegs * sizeof(XPoint))) != NULL)
			{
				for (h = 0; h < rowheight; h++)
				{
					count = 0;
					prevx = -1;
					for (i=start; i < stop; i++)
					{
						pts[count].x = offx + (data[i] - winstart) * pixels_per_ms;
						if (pts[count].x == prevx)
							continue;
						pts[count].y = y + h;
						prevx = pts[count].x;
						count++;
					}
					XDrawPoints(dpy, pixmap, gc, pts, count, CoordModeOrigin);
				}
				free(pts);
			}
		}
	}
	XSetClipMask(dpy, gc, None);
}


static void PSDrawRAST(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	float **repptrs, int *repcounts, int nreps, int samplerate,
	float xrangemin, float xrangemax,
	int ymin, int ymax, int dotshape)
{
	PSPoint *pts;
	float pixels_per_ms, pixels_per_row, winstart, winstop;
	int i, nsegs, rep;
	float y;
	float *data;
	int count, start, stop, prevx;
	int rowheight, rowspacing;

	if ((repptrs == NULL) || (repcounts == NULL) || (nreps == 0) || (ymin >= ymax) || (xrangemin >= xrangemax) || (height < 1))
		return;

	switch (dotshape)
	{
		case 1: rowheight = 1; rowspacing = 0; break;
		case 2: rowheight = 1; rowspacing = 1; break;
		case 3: rowheight = 2; rowspacing = 0; break;
		case 4: rowheight = 2; rowspacing = 1; break;
		case 5: rowheight = 3; rowspacing = 0; break;
		case 6: rowheight = 3; rowspacing = 1; break;
		case 7: rowheight = 4; rowspacing = 0; break;
		case 8: rowheight = 4; rowspacing = 1; break;
		case 9: rowheight = 8; rowspacing = 0; break;
		case 10: rowheight = 8; rowspacing = 1; break;
		default: rowheight = 1; rowspacing = 0; break;
	}

	winstart = xrangemin;
	winstop = xrangemax;
	pixels_per_ms = ((float)width) / (xrangemax - xrangemin);
	pixels_per_row = ((float)height) / ((float)(ymax - ymin));

	if (pixels_per_row > (rowheight + rowspacing))
		pixels_per_row = rowheight + rowspacing;

	fprintf(fp, "/rowheight %d def\n", rowheight);
	for (rep = 1; rep <= nreps; rep++)
	{
		data = repptrs[rep];
		count = repcounts[rep];
		start = 0;
		while ((start < count) && (data[start] < winstart)) start++;
		stop = start;
		while ((stop < count) && (data[stop] < winstop)) stop++;
		count = stop - start;
		if (count > 0)
		{
			y = offy2 + height - (rep) * pixels_per_row;

			nsegs = count;
			if ((pts = (PSPoint *)malloc(nsegs * sizeof(PSPoint))) != NULL)
			{
				count = 0;
				prevx = -1;
				for (i=start; i < stop; i++)
				{
					pts[count].x = offx + (data[i] - winstart) * pixels_per_ms;
					if (pts[count].x == prevx)
						continue;
					pts[count].y = y;
					prevx = pts[count].x;
					count++;
				}
				PSDrawSpikesFP(fp, pts, count);
				free(pts);
			}
		}
	}
}

