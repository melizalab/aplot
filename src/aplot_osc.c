
/**********************************************************************************
 * INCLUDES
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
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
#include <X11/cursorfont.h>
#include "aplot.h"
#include <Xm/XmP.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/CoreP.h>


/**********************************************************************************
 * STRUCTURES
 **********************************************************************************/
struct oscdata
{
	GC drawing_GC, inverse_GC, mark_GC;
	Pixmap pixmap;
	Boolean pixmapalloced;
	Boolean butgrabbed;
	int xmin,xmax,ymin,ymax;

	short min, max;
	Boolean minmaxcomputed;
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
#define CONV_TIME_TO_SAMPLE(samplerate, time) ((float)time * (samplerate / 1000.0))
#define CONV_SAMPLE_TO_TIME(samplerate, time) ((float)time * (1000.0 / samplerate))
#define CONV_VALUE_TO_Y(ymin, ymax, height, offy, value) (offy + height - (((value - ymin) / (ymax - ymin)) * (height)))
#define CONV_MS_TO_X(plot, t) \
	(((((t) - plot->group->xrangemin) / (plot->group->xrangemax - plot->group->xrangemin)) * (plot->width)) + plot->offx)


#define CreateMenuToggle(widget, parent, name, label, set, userdata) \
	widget = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNset, (set), 															\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNvalueChangedCallback, osc_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);

#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, osc_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int osc_open(PLOT *plot);
int osc_display(PLOT *plot);
int osc_set(PLOT *plot);
int osc_close(PLOT *plot);
int osc_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int osc_save(PLOT *plot);
int osc_play(PLOT *plot, int reqoffset, int reqsize, short **retsamples, int *retsize, float *playtime);
static void osc_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void osc_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void osc_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void osc_playmarker(PLOT *plot, float t);
static void osc_clearmarks(PLOT *plot);
static void osc_drawstartmark(PLOT *plot, float t);
static void osc_drawstopmark(PLOT *plot, float t);
static void osc_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);
static void DrawOscillo(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	short *samples, int nsamples, int xmin, int xmax, int ymin, int ymax);
static void PSDrawOscillo(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	short *samples, int nsamples, int xmin, int xmax, int ymin, int ymax);

float osc_conv_pixel_to_time(PLOT *plot, int x);
float osc_conv_pixel_to_val(PLOT *plot, int y);

/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int osc_open(PLOT *plot)
{
	struct oscdata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3, m4, m5, m6;
	int status = SUCCESS;
	Dimension width, height;
	int depth, offx, offy, offx2, offy2;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct oscdata));
	plotdata = (struct oscdata *)plot->plotdata;

	plot->plot_widget = XtVaCreateManagedWidget("osc", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.osc_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);

	XtAddCallback(plot->plot_widget, XmNexposeCallback, osc_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, osc_resize_callback, (XtPointer)plot);

	/* Register an event handler */
	XtAddEventHandler(plot->plot_widget, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
		FALSE, (XtEventHandler) osc_event_handler, (XtPointer)plot);

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

	plot->drawxaxis = defaults.osc_drawxaxis;
	plot->drawyaxis = defaults.osc_drawyaxis;

	offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
	offy = plot->offy = 0;
	offx2 = plot->offx2 = 0;
	offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
	plot->width = width - offx - offx2;
	plot->height = height - offy - offy2;

	plot->depth = depth;

	/*
	** Create the Graphics contexts.
	** drawing_GC is for the oscillograph itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
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
	CreateMenuToggle(m4, mw, "noiseclip", "NoiseClip", plot->noiseclip, "NoiseClip");
	CreateMenuButton(m5, mw, "save", "Save", "Save");
	CreateMenuButton(m6, mw, "print", "Print EPS", "Print EPS");

	plot->plot_playmarker = osc_playmarker;

	plot->plot_clearmarks = osc_clearmarks;
	plot->plot_drawstartmark = osc_drawstartmark;
	plot->plot_drawstopmark = osc_drawstopmark;
	plot->plot_conv_pixel_to_time = osc_conv_pixel_to_time;

	plot->plot_display = osc_display;
	plot->plot_set = osc_set;
	plot->plot_close = osc_close;
	plot->plot_print = osc_print;
	plot->plot_save = osc_save;

	plot->plot_event_handler = osc_event_handler;
	plot->plot_play = osc_play;

	plotdata->butgrabbed = FALSE;
	plot->playmark = -1;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needpcm = 1;

	return status;
}

int osc_display(PLOT *plot)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
	int status = SUCCESS, tstatus;
	int i, nsamples;
	short min, max, *samples;
	GROUP *group = plot->group;

	/*
	** Load the pcm data, but only if we need to...
	*/
	if (group->pcmdirty == 1)
	{
		group->samplerate = 20000;
		if (load_pcmdata(&group->pcmfp, group->loadedfilename, group->filename, group->entry, 
			&group->samples, &group->nsamples, &group->samplerate, &group->start, &group->stop, &group->ntime) == 0)
		{
			fprintf(stderr, "aplot osc: file has samplerate %d\n", group->samplerate);
			group->pcmdirty = 0;
			group->xrangemin = group->start;
			group->xrangemax = group->stop;
		}
		else status = ERROR;
	}

	nsamples = group->nsamples;
	samples = group->samples;

	plotdata->minmaxcomputed = FALSE;
	if (group->ymin == group->ymax)
	{
		max = SHRT_MIN;
		min = SHRT_MAX;
		for (i=0; i < nsamples; i++)
		{
			if (samples[i] > max) max = samples[i];
			if (samples[i] < min) min = samples[i];
		}
		plotdata->minmaxcomputed = TRUE;
		plotdata->min = min;
		plotdata->max = max;
	}
	tstatus = osc_set(plot);
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
int osc_set(PLOT *plot)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	short *samples, min, max;
	int i, nsamples, xmin, xmax, ymin, ymax, offx, offy, offx2, offy2;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to osc_open().
	*/
	if (plotdata->butgrabbed == FALSE)
	{
		XGrabButton(dpy, AnyButton, AnyModifier, XtWindow(w), True,
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
		defaults.osc_height = height;
		defaults.width = width;

		/*
		** Set the axes margins (note: if you want to change
		** the ticklblfont, then you must also update the
		** plot->minoff?? numbers.  They are not recalculated
		** here.)  (see osc_open)
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
		samples = plot->group->samples;
		nsamples = plot->group->nsamples;

		/*
		** Calculate the bounds: the first and last sample value to plot, and the y-axis bounds.
		** Fix up the bounds if necessary.
		*/
		xmin = (plot->group->xrangemin - plot->group->start) * (plot->group->samplerate / 1000.0);
		xmax = (plot->group->xrangemax - plot->group->start) * (plot->group->samplerate / 1000.0);
		ymin = plot->group->ymin;
		ymax = plot->group->ymax;
		if (xmin == xmax) xmin = 0;
		if ((xmin == 0) && (xmax == 0)) xmax = nsamples;
		if (xmax > nsamples) xmax = nsamples;
		if (ymin == ymax)
		{
			/* We need to compute the min and max values.  Cache these. */
			if (plotdata->minmaxcomputed == TRUE)
			{
				ymin = min = plotdata->min;
				ymax = max = plotdata->max;
			}
			else
			{
				max = SHRT_MIN;
				min = SHRT_MAX;
				for(i=0; i < nsamples; i++)
				{
					if (samples[i] > max) max = samples[i];
					if (samples[i] < min) min = samples[i];
				}
				plotdata->minmaxcomputed = TRUE;
				ymin = plotdata->min = min;
				ymax = plotdata->max = max;
			}
		}
		plotdata->xmin = xmin;
		plotdata->xmax = xmax;
		plotdata->ymin = ymin;
		plotdata->ymax = ymax;

		/*
		** Clear any marks
		*/
		plot->playmark = -1;
		plot->markx1 = -1;
		plot->markx2 = -1;

		/*
		** Do the rendering to an off-screen pixmap.
		** Then set this pixmap to be the background of the window,
		** and with the XClearArea(), generate an expose event that
		** will tell the server to render the background.
		*/
		DrawOscillo(dpy, plotdata->pixmap, plotdata->drawing_GC, offx, offy, width, height,
			samples, nsamples, xmin, xmax, ymin, ymax);

		if (plot->drawxaxis == TRUE)
			DrawXAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
				offx, offy, width, height, plot->group->xrangemin, plot->group->xrangemax);

		if (plot->drawyaxis == TRUE)
			DrawYAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
				offx, offy, width, height, ymin, ymax);

/*		XSetWindowBackgroundPixmap(dpy, XtWindow(w), plotdata->pixmap);
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0, 0, 0, True); */

		XCopyArea(dpy, plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
			0, 0, width + offx + offx2, height + offy + offy2, 0, 0);
	}
	return SUCCESS;
}

int osc_close(PLOT *plot)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
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

int osc_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
	Dimension width, height;
	char filenamestore[256], *filename, *ch, tempstr[40];
	int xmin, xmax, ymin, ymax, offx, offy, offx2, offy2;
	short *samples;
	int nsamples;
	int fileopenflag = 0;

	/*
	** Create the filename of the output file:
	** 1) start with the input filename
	** 2) remove the extension
	** 3) add the extension: _osc_<entry>_<startms>_<stopms>.eps
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->loadedfilename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_osc_%ld_%d_%d.eps", plot->group->entry, (int)plot->group->xrangemin, (int)plot->group->xrangemax);
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
		xmin = plotdata->xmin;
		xmax = plotdata->xmax;
		ymin = plotdata->ymin;
		ymax = plotdata->ymax;
		samples = plot->group->samples;
		nsamples = plot->group->nsamples;

		PSDrawOscillo(fp, offx, offy, offx2, offy2, width, height,
			samples, nsamples, xmin, xmax, ymin, ymax);
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

int osc_save(PLOT *plot)
{
	FILE *fp;
	short *samples;
	long numwritten;
	char filenamestore[256], *filename, *ch, tempstr[40];
	float tmin, tmax, ttmp;
	int start, stop;
 	PCMFILE *pcmfp;
 	unsigned long entrytimesecs;
 	unsigned long entrytimemicrosecs;

	if ((plot->markx1 != -1) && (plot->markx2 != -1))
	{
		tmin = osc_conv_pixel_to_time(plot, plot->markx1);
		tmax = osc_conv_pixel_to_time(plot, plot->markx2);
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
		if (tmin < plot->group->start) tmin = plot->group->start;
		if (tmax > plot->group->stop) tmax = plot->group->stop;
	}

	/*
	** Create the filename of the output file:
	** 1) start with the input filename
	** 2) remove the extension
	** 3) add the extension: _osc_<entry>_<startms>_<stopms>.pcm
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->loadedfilename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_osc_%ld_%d_%d", plot->group->entry, (int) tmin, (int) tmax);
	strcat(filename, tempstr);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '/')) ch--;
	if (*ch == '/') filename = ++ch;

 	start = CONV_TIME_TO_SAMPLE(plot->group->samplerate, tmin);
 	stop = CONV_TIME_TO_SAMPLE(plot->group->samplerate, tmax);
 	samples = plot->group->samples;
 
 	if (resource_data.outputseq2 == 1)
	{
		strcat(filename, ".pcm_seq2");
		if ((pcmfp = pcm_open(filename, "w")) != NULL)
		{
			entrytimesecs = 0;
			entrytimemicrosecs = 0;
			pcm_ctl(plot->group->pcmfp, PCMIOGETTIME, &entrytimesecs);
			pcm_ctl(plot->group->pcmfp, PCMIOGETTIMEFRACTION, &entrytimemicrosecs);
			entrytimesecs += (start / plot->group->samplerate);
			entrytimemicrosecs += (unsigned long)(start * (1000000.0 / plot->group->samplerate)) % 1000000;
			pcm_ctl(pcmfp, PCMIOSETSR, &(plot->group->samplerate));
			pcm_ctl(pcmfp, PCMIOSETTIME, &entrytimesecs);
			pcm_ctl(pcmfp, PCMIOSETTIMEFRACTION, &entrytimemicrosecs);
			if (pcm_write(pcmfp, samples + start, stop - start + 1) != -1)
				printf("OSC: saved %d samples into '%s'\n", stop-start+1, filename);
			else
				fprintf(stderr, "OSC: Error writing to '%s'\n", filename);
			pcm_close(pcmfp);
		}
		else fprintf(stderr, "OSC: Error opening '%s'\n", filename);
	}
 	else if (defaults.outputwav == TRUE)
	{
		strcat(filename, ".wav");
		if ((pcmfp = pcm_open(filename, "w")) != NULL)
		{
			entrytimesecs = 0;
			entrytimemicrosecs = 0;
			pcm_ctl(plot->group->pcmfp, PCMIOGETTIME, &entrytimesecs);
			pcm_ctl(plot->group->pcmfp, PCMIOGETTIMEFRACTION, &entrytimemicrosecs);
			entrytimesecs += (start / plot->group->samplerate);
			entrytimemicrosecs += (unsigned long)(start * (1000000.0 / plot->group->samplerate)) % 1000000;
			pcm_ctl(pcmfp, PCMIOSETSR, &(plot->group->samplerate));
			pcm_ctl(pcmfp, PCMIOSETTIME, &entrytimesecs);
			pcm_ctl(pcmfp, PCMIOSETTIMEFRACTION, &entrytimemicrosecs);
			if (pcm_write(pcmfp, samples + start, stop - start + 1) != -1)
				printf("OSC: saved %d samples into '%s'\n", stop-start+1, filename);
			else
				fprintf(stderr, "OSC: Error writing to '%s'\n", filename);
			pcm_close(pcmfp);
		}
		else fprintf(stderr, "OSC: Error opening '%s'\n", filename);
	}
 	else
	{
		strcat(filename, ".pcm");
		if ((fp = fopen(filename, "w")) != NULL)
		{
			numwritten = fwrite(samples + start, sizeof(short), stop - start + 1, fp);
			printf("OSC: saved %ld samples into '%s'\n", numwritten, filename);
			fclose(fp);
		}
		else fprintf(stderr, "OSC: Error opening '%s'\n", filename);
	}
	return SUCCESS;
}


int osc_play(PLOT *plot, int reqoffset, int reqsize, short **retsamples, int *retsize, float *playtime)
{
	float tmin, tmax, ttmp;
	int start, stop;

	*retsamples = NULL;
	*retsize = 0;

	if ((plot->markx1 != -1) && (plot->markx2 != -1))
	{
		tmin = osc_conv_pixel_to_time(plot, plot->markx1);
		tmax = osc_conv_pixel_to_time(plot, plot->markx2);
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
		if (tmin < plot->group->start) tmin = plot->group->start;
		if (tmax > plot->group->stop) tmax = plot->group->stop;
	}

	start = CONV_TIME_TO_SAMPLE(plot->group->samplerate, tmin);
	stop = CONV_TIME_TO_SAMPLE(plot->group->samplerate, tmax);
	start += reqoffset;
	if (stop > plot->group->nsamples)
		stop = plot->group->nsamples;
	if (start >= stop)
		return SUCCESS;

	*retsamples = &(plot->group->samples[start]);
	*retsize = (reqsize > (stop - start)) ? (stop - start) : (reqsize);
	*playtime = CONV_SAMPLE_TO_TIME(plot->group->samplerate, start);
	return SUCCESS;
}


static void osc_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
	XExposeEvent *expevent;
	int x, y0,y1;

	report_size(plot, " OSC EXPOSE");

	/*
	** Take care of the actual oscillograph.  Then,
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
		if ((x = plot->playmark) != -1)
		{
			if ((x >= expevent->x) && (x <= (expevent->x + expevent->width)))
			{
				y0 = MAX(expevent->y, plot->offy);
				y1 = MIN(expevent->y + expevent->height, plot->offy + plot->height);
				XDrawLine(XtDisplay(w), XtWindow(w), plotdata->mark_GC, x, y0, x, y1);
			}
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
static void osc_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	report_size(plot, " OSC RESIZE");
	osc_set(plot);
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

static void osc_playmarker(PLOT *plot, float t)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
	int x;

	if (plot->group->xrangemin >= plot->group->xrangemax)
		return;

	if ((t < 0.0) &&  (plot->playmark != -1))
	{
		DRAW_MARK(plot, plotdata, plot->playmark);			/* Erase existing mark */
		return;
	}
	x = CONV_MS_TO_X(plot, t);
	if (x != plot->playmark)								/* If there's no change, then do nothing */
	{
		if (IS_X_INBOUNDS(plot, x))
		{
			if (plot->playmark != -1)
				DRAW_MARK(plot, plotdata, plot->playmark);	/* Erase the old one */
			DRAW_MARK(plot, plotdata, x);					/* Draw the new one */
			plot->playmark = x;
		}
		flush_xevents();
	}
	return;
}

static void osc_clearmarks(PLOT *plot)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;

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

static void osc_drawstartmark(PLOT *plot, float t)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
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

static void osc_drawstopmark(PLOT *plot, float t)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;
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
static void osc_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
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
						defaults.osc_drawxaxis = plot->drawxaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "xaxis"), XmNset, plot->drawxaxis, NULL);
						osc_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'y'))
					{
						plot->drawyaxis = !(plot->drawyaxis);
						defaults.osc_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "yaxis"), XmNset, plot->drawyaxis, NULL);
						osc_set(plot);
					}
					else if (!strcmp(keybuf, "F2"))
					{
						int c;

						/*
						** Fork off a child
						*/
						if ((c = fork()) == 0)
						{
							FILE *pfp;
							int caps;
							char filenamebuf[256], *ch;

							fprintf(stderr, "You pressed Infer\n");

							strcpy(filenamebuf, plot->group->loadedfilename);
							ch = strrchr(filenamebuf, '.');
							if ((pcm_ctl(plot->group->pcmfp, PCMIOGETCAPS, &caps) != -1) && ((caps & PCMIOCAP_MULTENTRY) == 0))
								sprintf(ch, ".toe_lis");
							else
								sprintf(ch, "_%03d.toe_lis", (int)plot->group->entry);

							pfp = popen("amishss", "w");
							fprintf(pfp, "load %s %d\n", plot->group->loadedfilename, (int) plot->group->entry);
							fprintf(pfp, "infer\n");
							fprintf(pfp, "save aplottmp.ssetN\n");
							fprintf(pfp, "class\n");
							fprintf(pfp, "w %s\n", filenamebuf);
							pclose(pfp);
							exit(0);
						}
						if (c < 0)
						{
							fprintf(stderr, "aplot: cannot fork: %s\n", strerror(errno));
						}
					}
					else if (!strcmp(keybuf, "F3"))
					{
						int c;

						/*
						** Fork off a child
						*/
						if ((c = fork()) == 0)
						{
							FILE *pfp;
							int caps;
							char filenamebuf[256], *ch;

							fprintf(stderr, "You pressed Class\n");

							strcpy(filenamebuf, plot->group->loadedfilename);
							ch = strrchr(filenamebuf, '.');
							if ((pcm_ctl(plot->group->pcmfp, PCMIOGETCAPS, &caps) != -1) && ((caps & PCMIOCAP_MULTENTRY) == 0))
								sprintf(ch, ".toe_lis");
							else
								sprintf(ch, "_%03d.toe_lis", (int)plot->group->entry);

							pfp = popen("amishss", "w");
							fprintf(pfp, "sload aplottmp.ssetN\n");
							fprintf(pfp, "load %s %d\n", plot->group->loadedfilename, (int) plot->group->entry);
							fprintf(pfp, "class\n");
							fprintf(pfp, "w %s\n", filenamebuf);
							pclose(pfp);
							exit(0);
						}
						if (c < 0)
						{
							fprintf(stderr, "aplot: cannot fork: %s\n", strerror(errno));
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
						if (butevent->state & ShiftMask)
						{
							ttmp = osc_conv_pixel_to_time(plot, butevent->x);
							tmin = ttmp - 150;
							tmax = ttmp + 150;
							if (tmin >= 0)
								panel_zoom(plot->panel, tmin, tmax);
						}
						else
						{
							char title[256];
							sprintf(title, "%.2f", osc_conv_pixel_to_time(plot, butevent->x));
							if (IS_Y_INBOUNDS(plot, butevent->y))
								sprintf(title + strlen(title), " (%.0f)", osc_conv_pixel_to_val(plot, butevent->y));
							XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
						}
					}
					break;

				case Button2:
					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					/* Now draw and set the start mark */
					panel_drawstartmark(plot->panel, osc_conv_pixel_to_time(plot, butevent->x));

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", osc_conv_pixel_to_time(plot, butevent->x));
						sprintf(title + strlen(title), " : %.2f (0)", osc_conv_pixel_to_time(plot, butevent->x));
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
					panel_drawstopmark(plot->panel, osc_conv_pixel_to_time(plot, x));

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
				panel_drawstopmark(plot->panel, osc_conv_pixel_to_time(plot, x));
				if (IS_X_INBOUNDS(plot, motevent->x))
				{
					char title[256];
					float t1, t2;
					t1 = osc_conv_pixel_to_time(plot, MIN(plot->markx1, motevent->x));
					t2 = osc_conv_pixel_to_time(plot, MAX(plot->markx1, motevent->x));
					sprintf(title, "%.2f : %.2f (%.2f)", t1, t2, (t2 - t1));
					XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
				}
			}
			break;

		default:
			break;
	}
	if (b) *b = FALSE;
}

static void osc_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	char *idstring;
	Boolean drawxaxis, drawyaxis, noiseclip;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "X axis"))
	{
		XtVaGetValues(w, XmNset, &drawxaxis, NULL);
		plot->drawxaxis = drawxaxis;
		defaults.osc_drawxaxis = plot->drawxaxis;
		osc_set(plot);
	}
	else if (!strcmp(idstring, "Y axis"))
	{
		XtVaGetValues(w, XmNset, &drawyaxis, NULL);
		plot->drawyaxis = drawyaxis;
		defaults.osc_drawyaxis = plot->drawyaxis;
		osc_set(plot);
	}
	else if (!strcmp(idstring, "NoiseClip"))
	{
		XtVaGetValues(w, XmNset, &noiseclip, NULL);
		plot->noiseclip = noiseclip;
	}
	else if (!strcmp(idstring, "Save"))
	{
		osc_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		osc_print(plot, NULL, NULL);
	}
}


/*
** For the following, I employ the following notation:
** X, Y refer to pixel coordinates.
** TIME, VAL are in milliseconds and A/D converter units, respectively.
** SAMPLE is time in samples
*/
float osc_conv_pixel_to_time(PLOT *plot, int x)
{
	return ((((float)(x - plot->offx) / (float)(plot->width)) * 
			(float)(plot->group->xrangemax - plot->group->xrangemin)) + (float)plot->group->xrangemin);
}

float osc_conv_pixel_to_val(PLOT *plot, int y)
{
	struct oscdata *plotdata = (struct oscdata *)plot->plotdata;

	return (((float)plotdata->ymax - (((float)(y - plot->offy) / \
			(float)(plot->height)) * (float)(plotdata->ymax - plotdata->ymin))));
}


/*
** draws the oscillograph in a pixmap
**
** dpy									== needed for XDrawSegments
** pixmap								== must be already created
** gc									== for drawing the line segments
** offx,offy,height,width				== defines a rectangle in which to draw
** samples, nsamples					== the signed short raw-data samples
** xmin,xmax,ymin,ymax					== the axis bounds
*/
static void DrawOscillo(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height,
	short *samples, int nsamples, int xmin, int xmax, int ymin, int ymax)
{
	XSegment *segs;
	XPoint *pts;
	short *pcm, pcmmin, pcmmax;
	float samples_per_pixel, pixels_per_sample, pixels_per_yrange;
	int i, j, nsegs, yzero;
	float xstart, xstop;
	XRectangle clipbox;

	if ((samples == NULL) || (nsamples == 0) || (ymin >= ymax) || (xmin >= xmax) || (height < 10))
		return;

	clipbox.x = 0;
	clipbox.y = 0;
	clipbox.width = width;
	clipbox.height = height;
	XSetClipRectangles(dpy, gc, offx, offy, &clipbox, 1, Unsorted);

	pcm = samples + xmin;
	yzero = offy + (height * ymax /  (ymax - ymin));
	pixels_per_sample = ((float)width) / ((float)(xmax - xmin));
	pixels_per_yrange = ((float)height) / ((float)(ymax - ymin));
	if (pixels_per_sample < 0.2)
	{
		nsegs = width;
		if ((segs = (XSegment *)malloc(nsegs * sizeof(XSegment))) != NULL)
		{
			samples_per_pixel = 1.0 / pixels_per_sample;
			for (i=0; i < nsegs; i++)
			{
				pcmmin = SHRT_MAX;
				pcmmax = SHRT_MIN;
				xstart = i * samples_per_pixel;
				xstop = MIN(xmax - xmin, xstart + samples_per_pixel + 1);
				for (j=(int)xstart; j < (int)xstop; j++)
				{
					pcmmin = MIN(pcmmin, pcm[j]);
					pcmmax = MAX(pcmmax, pcm[j]);
				}
				segs[i].x1 = segs[i].x2 = (short) i + offx;
				segs[i].y1 = yzero - pixels_per_yrange * pcmmax;
				segs[i].y2 = yzero - pixels_per_yrange * pcmmin;
			}
			XDrawSegments(dpy, pixmap, gc, segs, nsegs);
			free(segs);
		}
	}
	else
	{
		nsegs = xmax - xmin + 1;
		if ((pts = (XPoint *)malloc(nsegs * sizeof(XPoint))) != NULL)
		{
			for (i=0; i < nsegs; i++)
			{
				pts[i].x = offx + (i * pixels_per_sample);
				pts[i].y = yzero - pcm[i] * pixels_per_yrange;
			}
			XDrawLines(dpy, pixmap, gc, pts, nsegs, CoordModeOrigin);
			free(pts);
		}
	}
	XSetClipMask(dpy, gc, None);
}


#define PRINTER_DPI 600.0

static void PSDrawOscillo(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height,
	short *samples, int nsamples, int xmin, int xmax, int ymin, int ymax)
{
	PSSegment *segs;
	PSPoint *pts;
	short *pcm, pcmmin, pcmmax;
	float samples_per_pixel, pixels_per_sample, pixels_per_yrange;
	int i, j, nsegs, yzero, newwidth, newheight;
	float xstart, xstop;

	if ((samples == NULL) || (nsamples == 0) || (ymin >= ymax) || (xmin >= xmax) || (height < 10))
		return;

	pcm = samples + xmin;
	newwidth = width * PRINTER_DPI / 72.0;
	newheight = height * PRINTER_DPI / 72.0;
	yzero = newheight - (newheight * ymax /  (ymax - ymin));

	fprintf(fp, "gsave\n");
	fprintf(fp, "%d %d translate\n", offx, offy2);
	fprintf(fp, "%f %f scale\n", 72.0 / PRINTER_DPI, 72.0 / PRINTER_DPI);

	pixels_per_sample = ((float)newwidth) / ((float)(xmax - xmin));
	pixels_per_yrange = ((float)newheight) / ((float)(ymax - ymin));
	if (pixels_per_sample < 0.1)
	{
		nsegs = newwidth;
		if ((segs = (PSSegment *)malloc(nsegs * sizeof(PSSegment))) != NULL)
		{
			samples_per_pixel = 1.0 / pixels_per_sample;
			for (i=0; i < nsegs; i++)
			{
				pcmmin = SHRT_MAX;
				pcmmax = SHRT_MIN;
				xstart = i * samples_per_pixel;
				xstop = xstart + samples_per_pixel;
				xstop = MIN(xmax - xmin, xstart + samples_per_pixel + 1);
				for (j=(int)xstart; j < (int)xstop; j++)
				{
					pcmmin = MIN(pcmmin, pcm[j]);
					pcmmax = MAX(pcmmax, pcm[j]);
				}
				segs[i].x1 = segs[i].x2 = i;
				segs[i].y1 = yzero + pixels_per_yrange * pcmmax;
				segs[i].y2 = yzero + pixels_per_yrange * pcmmin;
			}
			PSDrawSegmentsFP(fp, segs, nsegs);
			free(segs);
		}
	}
	else
	{
		nsegs = xmax - xmin + 1;
		if ((pts = (PSPoint *)malloc(nsegs * sizeof(PSPoint))) != NULL)
		{
			for (i=0; i < nsegs; i++)
			{
				pts[i].x = i * pixels_per_sample;
				pts[i].y = yzero + pcm[i] * pixels_per_yrange;
			}
			PSDrawLinesFP(fp, pts, nsegs);
			free(pts);
		}
	}

	fprintf(fp, "grestore\n");
}

