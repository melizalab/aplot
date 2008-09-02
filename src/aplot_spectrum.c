
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
#include <X11/Xatom.h>
#include <time.h>


/**********************************************************************************
 * STRUCTURES
 **********************************************************************************/
struct spectrumdata
{
	GC drawing_GC, inverse_GC, mark_GC;
	Pixmap pixmap;
	Boolean pixmapalloced;
	Boolean butgrabbed;
	int xmin,xmax;

	struct sonogram *sono;
	int ntimebins, nfreqbins;
	int fftsize, overlap;
	int nsamples;
	float gsmin, gsmax;
	float min, max;
	Boolean minmaxcomputed;
	Boolean drawxgrid, drawygrid;
	int ncolors;
	Colormap cmap;
	unsigned long colors[MAXCOLORS];
	XCC xcc;
	int method;                                                  /* FFT or MEM (maximum entropy) */
	int windowtype;                                              /* see sonogram.h for options */
	int npoles;                                                  /* parameter for MEM method */
	int scale;                                                   /* linear/dB scale */
	int nwin;                                                    /* MTM parameter */
	int npi;                                                     /* MTM parameter */
	int inoise;                                                  /* MTM parameter */
	int ilog;                                                    /* MTM parameter */
	int itype;                                                   /* MTM parameter */
	int ithresh;                                                 /* MTM parameter */
	int isignal;                                                 /* MTM parameter */
	int colormap;												 /* Type of colormap */

	int yscaling;
	int yscale_x0, yscale_y0;
};


/**********************************************************************************
 * DEFINES
 **********************************************************************************/
#define SONO_DEFAULT_MAX_COLORS 64
#define MAXCOLORS 256
#define RESERVED_COLORS 32
#define BRIGHTEST 65535


/*
** checking for something to be INBOUNDS means checking if a pixel in the window falls
** within the boundaries of the plot (as opposed to being in the axis area, for example.
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
	XtAddCallback(widget, XmNvalueChangedCallback, spectrum_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, spectrum_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int spectrum_open(PLOT *plot);
int spectrum_display(PLOT *plot);
int spectrum_set(PLOT *plot);
int spectrum_close(PLOT *plot);
int spectrum_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int spectrum_save(PLOT *plot);
static void spectrum_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void spectrum_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void spectrum_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void spectrum_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);
static void ASCII85_output(FILE *fp, int unsigned long word, int count);
static void DrawSpectrum(Display *dpy, Pixmap pixmap, GC gc, unsigned long *colors, int ncolors, int depth,
	int offx, int offy, int width, int height,
	struct sonogram *sono, short *samples, int nsamples,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float gsmin, float gsmax, float fmin, float fmax);
static void PSDrawSpectrum(FILE *fp, int ncolors,
	int offx, int offy, int offx2, int offy2, int width, int height,
	struct sonogram *sono, short *samples, int nsamples,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float gsmin, float gsmax, float fmin, float fmax);

float spectrum_conv_pixel_to_time(PLOT *plot, int x);
float spectrum_conv_pixel_to_yval(PLOT *plot, int y);


/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int spectrum_open(PLOT *plot)
{
	struct spectrumdata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3, m4, m5, m8, m9;
	Widget m6, m6a, m6a1, m6a2, m6a3, m6a4, m6a5, m6a6;
	Widget m7, m7a, m7a1, m7a2, m7a3, m7a4, m7a5;
	Widget m10, m10a, m10a1, m10a2, m10a3, m10a4;
	Widget m11, m11a, m11a1, m11a2;
	Widget m12, m12a, m12a1, m12a2, m12a3, m12a4, m12a5;
	Widget m13, m13a, m13a1, m13a2, m13a3, m13a4, m13a5, m13a6, m13a7, m13a8;
	Widget m14, m14a, m14a1, m14a2, m14a3, m14a4, m14a5, m14a6, m14a7, m14a8;
	Widget m15, m15a, m15a1, m15a2, m15a3, m15a4, m15a5, m15a6, m15a7, m15a8;
	Widget m16, m16a, m16a1, m16a2, m16a3;
	Widget m17, m17a, m17a1, m17a2;
	Widget m18, m18a, m18a1, m18a2;
	Widget m19, m19a, m19a1, m19a2, m19a3, m19a4, m19a5;
	Widget m20, m20a, m20a1, m20a2, m20a3;
	Widget m21, m21a, m21a1, m21a2, m21a4, m21a5;
	int status = SUCCESS;
	Dimension width, height;
	int depth, offx, offy, offx2, offy2;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct spectrumdata));
	plotdata = (struct spectrumdata *)plot->plotdata;

	plotdata->method = defaults.method;
	plotdata->windowtype = defaults.windowtype;
	plotdata->scale = defaults.scale;
	plotdata->npoles = defaults.npoles;
	plotdata->nwin = 3;
	plotdata->npi = 2;
	plotdata->inoise = 0;
	plotdata->ilog = 0;
	plotdata->itype = 1;
	plotdata->ithresh = 3;
	plotdata->isignal = 0;
	plotdata->colormap = defaults.colormap;

	plot->plot_widget = XtVaCreateManagedWidget("spectrum", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.spectrum_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);
	XtAddCallback(plot->plot_widget, XmNexposeCallback, spectrum_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, spectrum_resize_callback, (XtPointer)plot);

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
	** Store these margins for use later (we might start off without axes, and turn them on later.
	*/
	plot->ticklblfont = XmFontListCopy(_XmGetDefaultFontList(w, XmLABEL_FONTLIST));
	plot->minoffx = 6 + XmStringWidth(plot->ticklblfont, xmstr = XmStringCreateSimple("-32768")); XmStringFree(xmstr);
	plot->minoffx2 = 0;
	plot->minoffy = 0;
	plot->minoffy2 = 6 + XmStringHeight(plot->ticklblfont, xmstr = XmStringCreateSimple("1")); XmStringFree(xmstr);

	plot->drawxaxis = defaults.spectrum_drawxaxis;
	plot->drawyaxis = defaults.spectrum_drawyaxis;
	plotdata->drawxgrid = plot->group->sonoxgrid;
	plotdata->drawygrid = plot->group->sonoygrid;

	offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
	offy = plot->offy = 0;
	offx2 = plot->offx2 = 0;
	offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
	plot->width = width - offx - offx2;
	plot->height = height - offy - offy2;
	plot->depth = depth;

	/*
	** Allocate our colors.  We use the XCC code that is:
	** Copyright 1994,1995 John L. Cwikla
	** This allows us to work on any visual, etc.   The danger is that
	** we may not get the exact colors we ask for... meaning that things
	** may not really be as we see them.  This is unfortunate, but the
	** alternative is to not run at all (the old code crashed).  So...
	*/
	plotdata->ncolors = MIN(SONO_DEFAULT_MAX_COLORS, MIN(XDisplayCells(dpy, XDefaultScreen(dpy)), MAXCOLORS - RESERVED_COLORS));
	plotdata->xcc = XCCCreate(dpy, DefaultVisual(dpy, DefaultScreen(dpy)), TRUE, TRUE, XA_RGB_GRAY_MAP, &(plotdata->cmap));
	if (XCCGetNumColors(plotdata->xcc) < plotdata->ncolors)
	{
		plotdata->ncolors = XCCGetNumColors(plotdata->xcc);
		printf("Warning.  Using only %d colors.\n", plotdata->ncolors);
	}
	(*((colormap[plotdata->colormap]).cmap))(dpy, plotdata->ncolors, plotdata->colors, NULL);

	/*
	** Create the Graphics contexts.
	** drawing_GC is for the sonograph itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
	*/
	values.font = getFontStruct(plot->ticklblfont)->fid;
	values.function = GXcopy;
	values.plane_mask = AllPlanes;
	values.foreground = foreground;
	values.background = background;
	plotdata->drawing_GC = XtGetGC(w, GCFunction | GCPlaneMask | GCForeground | GCBackground | GCFont, &values);
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
	CreateMenuToggle(m4, mw, "xgrid", "X grid", plotdata->drawxgrid, "X grid");
	CreateMenuToggle(m5, mw, "ygrid", "Y grid", plotdata->drawygrid, "Y grid");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m6a = XmCreatePulldownMenu(mw, "m6a", args, n);
	m6 = XtVaCreateManagedWidget("fftsize", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("FFT size"),
		XmNsubMenuId, m6a,
		NULL);
	CreateMenuToggle(m6a1, m6a, "fftsize64", "64", (plot->group->fftsize == 64), "FFT size 64");
	CreateMenuToggle(m6a2, m6a, "fftsize128", "128", (plot->group->fftsize == 128), "FFT size 128");
	CreateMenuToggle(m6a3, m6a, "fftsize256", "256", (plot->group->fftsize == 256), "FFT size 256");
	CreateMenuToggle(m6a4, m6a, "fftsize512", "512", (plot->group->fftsize == 512), "FFT size 512");
	CreateMenuToggle(m6a5, m6a, "fftsize1024", "1024", (plot->group->fftsize == 1024), "FFT size 1024");
	CreateMenuToggle(m6a6, m6a, "fftsize2048", "2048", (plot->group->fftsize == 2048), "FFT size 2048");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m7a = XmCreatePulldownMenu(mw, "m7a", args, n);
	m7 = XtVaCreateManagedWidget("fftolap", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("FFT olap"),
		XmNsubMenuId, m7a,
		NULL);
	CreateMenuToggle(m7a1, m7a, "fftolap0", "0", (plot->group->fftolap == 0), "FFT olap 0");
	CreateMenuToggle(m7a2, m7a, "fftolap50", "50", (plot->group->fftolap == 50), "FFT olap 50");
	CreateMenuToggle(m7a3, m7a, "fftolap75", "75", (plot->group->fftolap == 75), "FFT olap 75");
	CreateMenuToggle(m7a4, m7a, "fftolap90", "90", (plot->group->fftolap == 90), "FFT olap 90");
	CreateMenuToggle(m7a5, m7a, "fftolap95", "95", (plot->group->fftolap == 95), "FFT olap 95");

	CreateMenuButton(m8, mw, "save", "Save", "Save");
	CreateMenuButton(m9, mw, "print", "Print EPS", "Print EPS");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m10a = XmCreatePulldownMenu(mw, "m10a", args, n);
	m10 = XtVaCreateManagedWidget("method", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("PSD method"),
		XmNsubMenuId, m10a,
		NULL);
	CreateMenuToggle(m10a1, m10a, "psdmethodFFT", "FFT", (plotdata->method == METHOD_FFT), "PSD method FFT");
	CreateMenuToggle(m10a2, m10a, "psdmethodMEM", "MEM", (plotdata->method == METHOD_MAXENTROPY), "PSD method MEM");
	CreateMenuToggle(m10a3, m10a, "psdmethodMTM", "MTM", (plotdata->method == METHOD_MULTITAPER), "PSD method MTM");
	CreateMenuToggle(m10a4, m10a, "psdmethodFFTW", "FFTW", (plotdata->method == METHOD_FFTW), "PSD method FFTW");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m11a = XmCreatePulldownMenu(mw, "m11a", args, n);
	m11 = XtVaCreateManagedWidget("scale", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Scale"),
		XmNsubMenuId, m11a,
		NULL);
	CreateMenuToggle(m11a1, m11a, "scaledB", "dB", (plotdata->scale == SCALE_DB), "SCALE dB");
	CreateMenuToggle(m11a2, m11a, "scaleLin", "Linear", (plotdata->scale == SCALE_LINEAR), "SCALE Linear");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m12a = XmCreatePulldownMenu(mw, "m12a", args, n);
	m12 = XtVaCreateManagedWidget("window", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Window"),
		XmNsubMenuId, m12a,
		NULL);
	CreateMenuToggle(m12a1, m12a, "windowhamming", "Hamming", (plotdata->windowtype == WINDOW_HAMMING), "WINDOW HAMMING");
	CreateMenuToggle(m12a2, m12a, "windowhanning", "Hanning", (plotdata->windowtype == WINDOW_HANNING), "WINDOW HANNING");
	CreateMenuToggle(m12a3, m12a, "windowboxcar", "Boxcar", (plotdata->windowtype == WINDOW_BOXCAR), "WINDOW BOXCAR");
	CreateMenuToggle(m12a4, m12a, "windowtriang", "Triang", (plotdata->windowtype == WINDOW_TRIANG), "WINDOW TRIANG");
	CreateMenuToggle(m12a5, m12a, "windowblackman", "Blackman", (plotdata->windowtype == WINDOW_BLACKMAN), "WINDOW BLACKMAN");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m21a = XmCreatePulldownMenu(mw, "m21a", args, n);
	m21 = XtVaCreateManagedWidget("colormap", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("Colormap"),
		XmNsubMenuId, m21a,
		NULL);
	CreateMenuToggle(m21a1, m21a, "colormapgray", "Gray", (plotdata->colormap == CMAP_GRAY), "CMAP GRAY");
	CreateMenuToggle(m21a2, m21a, "colormaphot", "Hot", (plotdata->colormap == CMAP_HOT), "CMAP HOT");
	CreateMenuToggle(m21a4, m21a, "colormapcool", "Cool", (plotdata->colormap == CMAP_COOL), "CMAP COOL");
	CreateMenuToggle(m21a5, m21a, "colormapbone", "Bone", (plotdata->colormap == CMAP_BONE), "CMAP BONE");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m13a = XmCreatePulldownMenu(mw, "m13a", args, n);
	m13 = XtVaCreateManagedWidget("npoles", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MEM npoles"),
		XmNsubMenuId, m13a,
		NULL);
	CreateMenuToggle(m13a1, m13a, "npoles10", "10", (plotdata->npoles == 10), "MEM npoles 10");
	CreateMenuToggle(m13a2, m13a, "npoles20", "20", (plotdata->npoles == 20), "MEM npoles 20");
	CreateMenuToggle(m13a3, m13a, "npoles30", "30", (plotdata->npoles == 30), "MEM npoles 30");
	CreateMenuToggle(m13a4, m13a, "npoles40", "40", (plotdata->npoles == 40), "MEM npoles 40");
	CreateMenuToggle(m13a5, m13a, "npoles50", "50", (plotdata->npoles == 50), "MEM npoles 50");
	CreateMenuToggle(m13a6, m13a, "npoles60", "60", (plotdata->npoles == 60), "MEM npoles 60");
	CreateMenuToggle(m13a7, m13a, "npoles70", "70", (plotdata->npoles == 70), "MEM npoles 70");
	CreateMenuToggle(m13a8, m13a, "npoles80", "80", (plotdata->npoles == 80), "MEM npoles 80");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m14a = XmCreatePulldownMenu(mw, "m14a", args, n);
	m14 = XtVaCreateManagedWidget("nwin", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MTM # windows"),
		XmNsubMenuId, m14a,
		NULL);
	CreateMenuToggle(m14a1, m14a, "nwin1", "1", (plotdata->nwin == 1), "MTM nwin 1");
	CreateMenuToggle(m14a2, m14a, "nwin2", "2", (plotdata->nwin == 2), "MTM nwin 2");
	CreateMenuToggle(m14a3, m14a, "nwin3", "3", (plotdata->nwin == 3), "MTM nwin 3");
	CreateMenuToggle(m14a4, m14a, "nwin4", "4", (plotdata->nwin == 4), "MTM nwin 4");
	CreateMenuToggle(m14a5, m14a, "nwin5", "5", (plotdata->nwin == 5), "MTM nwin 5");
	CreateMenuToggle(m14a6, m14a, "nwin6", "6", (plotdata->nwin == 6), "MTM nwin 6");
	CreateMenuToggle(m14a7, m14a, "nwin7", "7", (plotdata->nwin == 7), "MTM nwin 7");
	CreateMenuToggle(m14a8, m14a, "nwin8", "8", (plotdata->nwin == 8), "MTM nwin 8");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m15a = XmCreatePulldownMenu(mw, "m15a", args, n);
	m15 = XtVaCreateManagedWidget("npi", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MTM bandwidth"),
		XmNsubMenuId, m15a,
		NULL);
	CreateMenuToggle(m15a1, m15a, "npi1", "1", (plotdata->npi == 1), "MTM npi 1");
	CreateMenuToggle(m15a2, m15a, "npi2", "2", (plotdata->npi == 2), "MTM npi 2");
	CreateMenuToggle(m15a3, m15a, "npi3", "3", (plotdata->npi == 3), "MTM npi 3");
	CreateMenuToggle(m15a4, m15a, "npi4", "4", (plotdata->npi == 4), "MTM npi 4");
	CreateMenuToggle(m15a5, m15a, "npi5", "5", (plotdata->npi == 5), "MTM npi 5");
	CreateMenuToggle(m15a6, m15a, "npi6", "6", (plotdata->npi == 6), "MTM npi 6");
	CreateMenuToggle(m15a7, m15a, "npi7", "7", (plotdata->npi == 7), "MTM npi 7");
	CreateMenuToggle(m15a8, m15a, "npi8", "8", (plotdata->npi == 8), "MTM npi 8");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m16a = XmCreatePulldownMenu(mw, "m16a", args, n);
	m16 = XtVaCreateManagedWidget("inoise", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MTM noise assumption"),
		XmNsubMenuId, m16a,
		NULL);
	CreateMenuToggle(m16a1, m16a, "inoise0", "Red", (plotdata->inoise == 0), "MTM inoise 0");
	CreateMenuToggle(m16a2, m16a, "inoise1", "White", (plotdata->inoise == 1), "MTM inoise 1");
	CreateMenuToggle(m16a3, m16a, "inoise2", "Locally White", (plotdata->inoise == 2), "MTM inoise 2");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m17a = XmCreatePulldownMenu(mw, "m17a", args, n);
	m17 = XtVaCreateManagedWidget("ilog", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MTM noise fitting"),
		XmNsubMenuId, m17a,
		NULL);
	CreateMenuToggle(m17a1, m17a, "ilog0", "Log", (plotdata->ilog == 0), "MTM ilog 0");
	CreateMenuToggle(m17a2, m17a, "ilog1", "Linear", (plotdata->ilog == 1), "MTM ilog 1");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m18a = XmCreatePulldownMenu(mw, "m18a", args, n);
	m18 = XtVaCreateManagedWidget("mtmtype", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MTM type"),
		XmNsubMenuId, m18a,
		NULL);
	CreateMenuToggle(m18a1, m18a, "itype0", "Hi-res", (plotdata->itype == 0), "MTM itype 0");
	CreateMenuToggle(m18a2, m18a, "itype1", "Adaptive", (plotdata->itype == 1), "MTM itype 1");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m19a = XmCreatePulldownMenu(mw, "m19a", args, n);
	m19 = XtVaCreateManagedWidget("mtmtype", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MTM Threshold"),
		XmNsubMenuId, m19a,
		NULL);
	CreateMenuToggle(m19a1, m19a, "ithresh1", "90%", (plotdata->ithresh == 1), "MTM ithresh 1");
	CreateMenuToggle(m19a2, m19a, "ithresh2", "95%", (plotdata->ithresh == 2), "MTM ithresh 2");
	CreateMenuToggle(m19a3, m19a, "ithresh3", "99%", (plotdata->ithresh == 3), "MTM ithresh 3");
	CreateMenuToggle(m19a4, m19a, "ithresh4", "99.5%", (plotdata->ithresh == 4), "MTM ithresh 4");
	CreateMenuToggle(m19a5, m19a, "ithresh5", "99.9%", (plotdata->ithresh == 5), "MTM ithresh 5");

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, True); n++;
	m20a = XmCreatePulldownMenu(mw, "m20a", args, n);
	m20 = XtVaCreateManagedWidget("mtmtype", xmCascadeButtonWidgetClass, mw,
		XmNlabelString, xmstr1 = XmStringCreateSimple("MTM Signal Assumption"),
		XmNsubMenuId, m20a,
		NULL);
	CreateMenuToggle(m20a1, m20a, "isignal0", "Harm+QuasiPeriod", (plotdata->isignal == 0), "MTM isignal 0");
	CreateMenuToggle(m20a2, m20a, "isignal1", "QuasiPeriodic", (plotdata->isignal == 1), "MTM isignal 1");
	CreateMenuToggle(m20a3, m20a, "isignal2", "Harmonic", (plotdata->isignal == 2), "MTM isignal 2");

	/*
	** Register an event handler
	*/
	XtAddEventHandler(w, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | Button2MotionMask,
		FALSE, (XtEventHandler) spectrum_event_handler, (XtPointer)plot);

	plot->plot_playmarker = NULL;

	plot->plot_clearmarks = NULL;
	plot->plot_drawstartmark = NULL;
	plot->plot_drawstopmark = NULL;
	plot->plot_conv_pixel_to_time = spectrum_conv_pixel_to_time;

	plot->plot_display = spectrum_display;
	plot->plot_set = spectrum_set;
	plot->plot_close = spectrum_close;
	plot->plot_print = spectrum_print;

	plot->plot_save = spectrum_save;

	plot->plot_event_handler = spectrum_event_handler;
	plot->plot_play = NULL;

	plotdata->butgrabbed = FALSE;
	plot->playmark = -1;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needpcm = 1;

	return status;
}

int spectrum_display(PLOT *plot)
{
	struct spectrumdata *plotdata = (struct spectrumdata *)plot->plotdata;
	int status = SUCCESS, tstatus;
	struct sonogram *sono;
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
			group->pcmdirty = 0;
			group->xrangemin = group->start;
			group->xrangemax = group->stop;
		}
		else status = ERROR;
	}

	/*
	** Update the sonogram data...
	*/
	if (plotdata->sono != NULL)
		free_sonogram(plotdata->sono);
	if ((sono = create_sonogram()) == NULL)
	{
		fprintf(stderr, "WARNING: Error occurred while trying to create sonogram...\n");
		return ERROR;
	}
	sonogram_setopts(sono, SONO_OPT_FFTSIZE, group->fftsize);
	sonogram_setopts(sono, SONO_OPT_OVERLAP, group->fftolap);
	sonogram_setopts(sono, SONO_OPT_NSAMPLES, group->nsamples);
	sonogram_setopts(sono, SONO_OPT_METHOD, plotdata->method);
	sonogram_setopts(sono, SONO_OPT_NPOLES, plotdata->npoles);
	sonogram_setopts(sono, SONO_OPT_SCALE, plotdata->scale);
	sonogram_setopts(sono, SONO_OPT_WINDOW, plotdata->windowtype);
	sonogram_setopts(sono, SONO_OPT_MTM_NWIN, plotdata->nwin);
	sonogram_setopts(sono, SONO_OPT_MTM_NPI, plotdata->npi);
	sonogram_setopts(sono, SONO_OPT_MTM_INOISE, plotdata->inoise);
	sonogram_setopts(sono, SONO_OPT_MTM_ILOG, plotdata->ilog);
	sonogram_setopts(sono, SONO_OPT_MTM_ISPEC, plotdata->itype);
	sonogram_setopts(sono, SONO_OPT_MTM_ITHRESH, plotdata->ithresh);
	sonogram_setopts(sono, SONO_OPT_MTM_ISIGNAL, plotdata->isignal);

	plotdata->sono = sono;
	plotdata->ntimebins = sono->ntimebins;
	plotdata->nfreqbins = sono->nfreqbins;
	plotdata->fftsize = sono->fftsize;
	plotdata->overlap = sono->overlap;
	plotdata->nsamples = group->nsamples;

	if (group->frmin >= group->frmax)
	{
		group->frmin = 0;
		group->frmax = plot->group->samplerate / 2;
	}
	if (group->gsmin == group->gsmax)
	{
		group->gsmin = 0;
		group->gsmax = 100;
	}

	/*
	** Update the plot
	*/
	tstatus = spectrum_set(plot);
	return (status == SUCCESS) ? (tstatus) : (status);
}

int spectrum_set(PLOT *plot)
{
	struct spectrumdata *plotdata = (struct spectrumdata *)plot->plotdata;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	int xmin, xmax, ymin, ymax, offx, offy, offx2, offy2;
	float gsmin, gsmax, fmin, fmax;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to spectrum_open().
	*/
	if (plotdata->butgrabbed == FALSE)
	{
		XGrabButton(dpy, AnyButton, AnyModifier, XtWindow(w), TRUE,
			ButtonPressMask | ButtonReleaseMask | Button2MotionMask,
			GrabModeAsync, GrabModeAsync, XtWindow(w),
			XCreateFontCursor(dpy, XC_crosshair));
		plotdata->butgrabbed = TRUE;
	}

	if (!plotdata->pixmapalloced)
		return SUCCESS;

	/*
	** Retrieve the window size
	*/
	XtVaGetValues(w,
		XmNheight, &height,
		XmNwidth, &width,
		NULL);
	defaults.spectrum_height = height;
	defaults.width = width;

	/*
	** Set the axes margins (note: if you want to change
	** the ticklblfont, then you must also update the
	** plot->minoff?? numbers.  They are not recalculated
	** here.  (see spectrum_open)
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
	xmin = (plot->group->xrangemin - plot->group->start) * (plot->group->samplerate / 1000.0);
	xmax = (plot->group->xrangemax - plot->group->start) * (plot->group->samplerate / 1000.0);
	if (xmin == xmax) xmin = 0;
	if ((xmin == 0) && (xmax == 0)) xmax = plotdata->nsamples;
	if (xmax > plotdata->nsamples) xmax = plotdata->nsamples;
	plotdata->xmin = xmin;
	plotdata->xmax = xmax;
	ymin = plot->group->frmin;
	ymax = plot->group->frmax;
	gsmin = plot->group->gsmin;
	gsmax = plot->group->gsmax;
	fmin = ymin / (float)plot->group->samplerate;
	fmax = ymax / (float)plot->group->samplerate;

	/*
	** Clear any marks
	*/
	plot->playmark = -1;
	plot->markx1 = -1;
	plot->markx2 = -1;

	/*
	** Do the rendering to an off-screen pixmap, then copy to the window.
	*/
	DrawSpectrum(dpy, plotdata->pixmap, plotdata->drawing_GC, plotdata->colors,
		plotdata->ncolors, plot->depth, offx, offy, width, height,
		plotdata->sono, plot->group->samples, plot->group->nsamples,
		plot->group->xrangemin, plot->group->xrangemax,
		xmin, xmax, gsmin, gsmax, fmin, fmax);

	if (plotdata->drawxgrid == TRUE)
		DrawXGrid(dpy, plotdata->pixmap, plotdata->drawing_GC, offx, offy, width, height,
			plot->group->xrangemin, plot->group->xrangemax);

	if (plotdata->drawygrid == TRUE)
		DrawYGrid(dpy, plotdata->pixmap, plotdata->drawing_GC, offx, offy, width, height);

	if (plot->drawxaxis == TRUE)
		DrawXAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
			offx, offy, width, height, plot->group->xrangemin, plot->group->xrangemax);

	if (plot->drawyaxis == TRUE)
		DrawYAxis(dpy, plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
			offx, offy, width, height, ymin, ymax);

	XCopyArea(dpy, plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
		0, 0, width + offx + offx2, height + offy + offy2, 0, 0);

	return SUCCESS;
}

int spectrum_close(PLOT *plot)
{
	struct spectrumdata *plotdata = (struct spectrumdata *)plot->plotdata;
	Widget w = (Widget)plot->plot_widget;

	if (plotdata->sono != NULL) free_sonogram(plotdata->sono);
	if (plot->ticklblfont != NULL) XmFontListFree(plot->ticklblfont);
	if (plotdata->pixmapalloced) XFreePixmap(XtDisplay(w), plotdata->pixmap);
	if (plotdata->xcc) XCCFree(plotdata->xcc);
	XtReleaseGC(w, plotdata->drawing_GC);
	XtReleaseGC(w, plotdata->inverse_GC);
	XtReleaseGC(w, plotdata->mark_GC);
	XtDestroyWidget(plot->plot_widget);
	free(plot->plotdata);
	plot->plotdata = NULL;
	return SUCCESS;
}

int spectrum_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	struct spectrumdata *plotdata = (struct spectrumdata *)plot->plotdata;
	char filenamestore[256], *filename, *ch, tempstr[40];
	Dimension height, width;
	int ntimebins, nfreqbins, xmin, xmax, ymin, ymax, offx, offy, offx2, offy2;
	float gsmin, gsmax;
	int needtoclosefile;
	float fmin, fmax;

	/*
	** Create the filename of the output file:
	** 1) start with the input filename
	** 2) remove the extension
	** 3) add the extension: _spectrum_<entry>_<startms>_<stopms>.eps
	** 4) strip off the directory part of the filename
	*/
	filename = filenamestore;
	strcpy(filename, plot->group->loadedfilename);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '.')) ch--;
	if (*ch == '.') *ch = '\0';
	sprintf(tempstr, "_spectrum_%ld_%d_%d.eps", plot->group->entry, (int)plot->group->xrangemin, (int)plot->group->xrangemax);
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

	needtoclosefile = 0;
	if (fp == NULL)
	{
		fp = fopen(filename, "w");
		needtoclosefile = 1;
	}

	if (fp != NULL)
	{
		PSInit(fp, filename, width, height);

		offx = plot->offx = (plot->drawyaxis == TRUE) ? (plot->minoffx) : (0);
		offy = plot->offy = 0;
		offx2 = plot->offx2 = 0;
		offy2 = plot->offy2 = (plot->drawxaxis == TRUE) ? (plot->minoffy2) : (0);
		height = plot->height;
		width = plot->width;
		xmin = plotdata->xmin;
		xmax = plotdata->xmax;
		ymin = plot->group->frmin;
		ymax = plot->group->frmax;
		ntimebins = plotdata->ntimebins;
		nfreqbins = plotdata->nfreqbins;
		gsmin = plot->group->gsmin;
		gsmax = plot->group->gsmax;
		fmin = ymin / (float)plot->group->samplerate;
		fmax = ymax / (float)plot->group->samplerate;

		PSDrawSpectrum(fp, 256, offx, offy, offx2, offy2, width, height,
			plotdata->sono, plot->group->samples, plot->group->nsamples,
			plot->group->xrangemin, plot->group->xrangemax,
			xmin, xmax, gsmin, gsmax, fmin, fmax);

		if (plotdata->drawxgrid == TRUE)
			PSDrawXGrid(fp, offx, offy, offx2, offy2, width, height, plot->group->xrangemin, plot->group->xrangemax);
		if (plotdata->drawygrid == TRUE)
			PSDrawYGrid(fp, offx, offy, offx2, offy2, width, height);
		if (plot->drawxaxis == TRUE)
			PSDrawXAxis(fp, offx, offy, offx2, offy2, width, height, plot->group->xrangemin, plot->group->xrangemax);
		if (plot->drawyaxis == TRUE)
			PSDrawYAxis(fp, offx, offy, offx2, offy2, width, height, ymin, ymax);

		PSFinish(fp);
		if (needtoclosefile == 1)
			fclose(fp);
	}
	if (ret_filename_p) *ret_filename_p = strdup(filename);
	return SUCCESS;
}

int spectrum_save(PLOT *plot)
{
	char filenamestore[256], *filename, *ch, tempstr[40];
	float tmin, tmax;

	tmin = plot->group->xrangemin;
	tmax = plot->group->xrangemax;

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
	sprintf(tempstr, "_spectrum_%ld_%d_%d.pcm", plot->group->entry, (int)plot->group->xrangemin, (int)plot->group->xrangemax);
	strcat(filename, tempstr);
	ch = &(filename[strlen(filename)-1]);
	while ((ch > filename) && (*ch != '/')) ch--;
	if (*ch == '/') filename = ++ch;

	/*
	** Should save spectrum here...
	*/
	/* ... */
 	return SUCCESS;
}


static void spectrum_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct spectrumdata *plotdata = (struct spectrumdata *)plot->plotdata;
	XExposeEvent *expevent;
	int x, y0,y1;

	/*
	** The background pixmap will take care of the actual sonograph.  However,
	** we handle the marks here manually
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

static void spectrum_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	spectrum_set(plot);
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

/*
**  Button1 press: evaluate at curpos
**  Button2 press: clear marks.  start mark at curpos.
**  Button2 motion: clear old stop mark.  stop mark at curpos
**  Button2 release: same as motion.
*/
static void spectrum_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
{
	PLOT *plot = (PLOT *)clientData;
	struct spectrumdata *plotdata = (struct spectrumdata *)plot->plotdata;
	XButtonEvent *butevent;
	XMotionEvent *motevent;
	int x;
	XKeyEvent *keyevent;
	char *keybuf;
	KeySym key_sym;
	Modifiers mod_return;
	Widget ew;
	double y1, dy, py;

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
						defaults.spectrum_drawxaxis = plot->drawxaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "xaxis"), XmNset, plot->drawxaxis, NULL);
						spectrum_set(plot);
					}
					else if ((keyevent->state == ControlMask) && (keybuf[0] == 'y'))
					{
						plot->drawyaxis = !(plot->drawyaxis);
						defaults.spectrum_drawyaxis = plot->drawyaxis;
						XtVaSetValues(XtNameToWidget(plot->plot_popupmenu_widget, "yaxis"), XmNset, plot->drawyaxis, NULL);
						spectrum_set(plot);
					}
					else if ((keyevent->state == (ControlMask|ShiftMask)) && (keybuf[0] == 'x'))
					{
						plotdata->drawxgrid = !(plotdata->drawxgrid);
						spectrum_set(plot);
					}
					else if ((keyevent->state == (ControlMask|ShiftMask)) && (keybuf[0] == 'y'))
					{
						plotdata->drawygrid = !(plotdata->drawygrid);
						spectrum_set(plot);
					}
					else
					{
						panel_handle_keyrelease(w, plot, keyevent, keybuf);
					}
				}
			}
			break;

		case ButtonPress:
			butevent = (XButtonEvent *) event;
			switch (butevent->button)
			{
				case Button1:
					if (butevent->x < plot->offx)
					{
						plotdata->yscaling = 1;
						plotdata->yscale_x0 = butevent->x;
						plotdata->yscale_y0 = butevent->y;
						break;
					}

					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", spectrum_conv_pixel_to_time(plot, butevent->x));
						if (IS_Y_INBOUNDS(plot, butevent->y))
							sprintf(title + strlen(title), " (%.0f)", spectrum_conv_pixel_to_yval(plot, butevent->y));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
					break;

				case Button2:
					/* If there are any marks, erase them */
					panel_clearmarks(plot->panel);

					/* Now draw and set the start mark */
					panel_drawstartmark(plot->panel, spectrum_conv_pixel_to_time(plot, butevent->x));

					if (IS_X_INBOUNDS(plot, butevent->x))
					{
						char title[256];
						sprintf(title, "%.2f", spectrum_conv_pixel_to_time(plot, butevent->x));
						sprintf(title + strlen(title), " : %.2f (0)", spectrum_conv_pixel_to_time(plot, butevent->x));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
					break;

				case Button3:
					if (butevent->x < plot->offx)
					{
						plotdata->yscaling = 2;
						plotdata->yscale_x0 = butevent->x;
						plotdata->yscale_y0 = butevent->y;
						break;
					}
					XmMenuPosition(plot->plot_popupmenu_widget, butevent);
					XtManageChild(plot->plot_popupmenu_widget);
					break;

				default:
					break;
			}
			break;

		case ButtonRelease:
			if (plotdata->yscaling != 0)
			{
				plotdata->yscaling = 0;
				spectrum_set(plot);
				break;
			}
			butevent = (XButtonEvent *) event;
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
					panel_drawstopmark(plot->panel, spectrum_conv_pixel_to_time(plot, x));

					/* If the two marks are on top of each other, they're already erased.  Clear both marks. */
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
			if (motevent->state & Button1Mask)
			{
				if (plotdata->yscaling == 1)
				{
					y1 = motevent->y;
					dy = (y1 - plotdata->yscale_y0) / plot->height;
					if (dy > 0)
					{
						dy /= 100;
						dy = MIN(dy, 0.9) + 0.1;
						py = (plot->group->frmax - plot->group->frmin) * 10.0 * dy;
					}
					else
					{
						dy /= 20;
						dy *= -1;
						dy = 1.0 - MIN(dy, 0.9);
						py = (plot->group->frmax - plot->group->frmin) * 1.0 * dy;
					}
					plot->group->frmax = plot->group->frmax + py;
					plot->group->frmax = MAX(10, MIN(plot->group->frmax, plot->group->samplerate / 2));

					if (plot->drawyaxis == TRUE)
					{
						XFillRectangle(XtDisplay(w), plotdata->pixmap, plotdata->inverse_GC, 0, 0, plot->offx, plot->height + plot->offy + plot->offy2);
						DrawYAxis(XtDisplay(w), plotdata->pixmap, plotdata->drawing_GC, plot->ticklblfont,
							plot->offx, plot->offy, plot->width, plot->height,
							plot->group->frmin, plot->group->frmax);
						XCopyArea(XtDisplay(w), plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
							0, 0, plot->offx, plot->height + plot->offy + plot->offy2, 0, 0);
					}
				}
			}
			else if (motevent->state & Button2Mask)
			{
				if (plot->markx1 != -1)
				{
					x = MAX(motevent->x, plot->offx);
					x = MIN(x, plot->offx + plot->width);

					/* Erase the old stop mark, and draw the new one */
					panel_drawstopmark(plot->panel, spectrum_conv_pixel_to_time(plot, x));

					if (IS_X_INBOUNDS(plot, motevent->x))
					{
						char title[256];
						float t1, t2;
						t1 = spectrum_conv_pixel_to_time(plot, MIN(plot->markx1, motevent->x));
						t2 = spectrum_conv_pixel_to_time(plot, MAX(plot->markx1, motevent->x));
						sprintf(title, "%.2f : %.2f (%.2f)", t1, t2, (t2 - t1));
						XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
					}
				}
			}
			else if (motevent->state & Button2Mask)
			{
				if (plotdata->yscaling == 2)
				{
				}
			}
			break;

		default:
			break;
	}
}


static void spectrum_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	struct spectrumdata *plotdata = (struct spectrumdata *)plot->plotdata;
	char *idstring;
	Boolean drawxaxis, drawyaxis, drawxgrid, drawygrid, value;
	int fftsize, fftolap, windowtype = DEFAULT_WINDOWTYPE, npoles = DEFAULT_NPOLES, method = DEFAULT_METHOD, scale = DEFAULT_SCALE;
	int nwin, npi, inoise, ilog, itype, ithresh, isignal, cmap;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "X axis"))
	{
		XtVaGetValues(w, XmNset, &drawxaxis, NULL);
		plot->drawxaxis = drawxaxis;
		defaults.spectrum_drawxaxis = drawxaxis;
		spectrum_set(plot);
	}
	else if (!strcmp(idstring, "Y axis"))
	{
		XtVaGetValues(w, XmNset, &drawyaxis, NULL);
		plot->drawyaxis = drawyaxis;
		defaults.spectrum_drawyaxis = plot->drawyaxis;
		spectrum_set(plot);
	}
	else if (!strcmp(idstring, "X grid"))
	{
		XtVaGetValues(w, XmNset, &drawxgrid, NULL);
		plotdata->drawxgrid = drawxgrid;
		spectrum_set(plot);
	}
	else if (!strcmp(idstring, "Y grid"))
	{
		XtVaGetValues(w, XmNset, &drawygrid, NULL);
		plotdata->drawygrid = drawygrid;
		spectrum_set(plot);
	}
	else if (!strncmp(idstring, "FFT size", 8))
	{
		sscanf(idstring, "FFT size %d", &fftsize);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plot->group->fftsize != fftsize))
		{
			plot->group->fftsize = fftsize;
			defaults.fftsize = fftsize;
			spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "PSD method", 10))
	{
		if (!strcmp(idstring, "PSD method FFT"))
			method = METHOD_FFT;
		else if (!strcmp(idstring, "PSD method MEM"))
			method = METHOD_MAXENTROPY;
		else if (!strcmp(idstring, "PSD method MTM"))
			method = METHOD_MULTITAPER;
		else if (!strcmp(idstring, "PSD method FFTW"))
			method = METHOD_FFTW;
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->method != method))
		{
			plotdata->method = method;
			defaults.method = method;
			spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "SCALE", 5))
	{
		if (!strcmp(idstring, "SCALE dB"))
			scale = SCALE_DB;
		else if (!strcmp(idstring, "SCALE Linear"))
			scale = SCALE_LINEAR;
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->scale != scale))
		{
			plotdata->scale = scale;
			defaults.scale = scale;
			spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "WINDOW", 6))
	{
		if (!strcmp(idstring, "WINDOW HAMMING"))
			windowtype = WINDOW_HAMMING;
		else if (!strcmp(idstring, "WINDOW HANNING"))
			windowtype = WINDOW_HANNING;
		else if (!strcmp(idstring, "WINDOW BOXCAR"))
			windowtype = WINDOW_BOXCAR;
		else if (!strcmp(idstring, "WINDOW TRIANG"))
			windowtype = WINDOW_TRIANG;
		else if (!strcmp(idstring, "WINDOW BLACKMAN"))
			windowtype = WINDOW_BLACKMAN;
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->windowtype != windowtype))
		{
			plotdata->windowtype = windowtype;
			defaults.windowtype = windowtype;
			spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "CMAP", 4))
	{
		cmap = CMAP_GRAY;
		if (!strcmp(idstring, "CMAP GRAY"))
			cmap = CMAP_GRAY;
		else if (!strcmp(idstring, "CMAP HOT"))
			cmap = CMAP_HOT;
		else if (!strcmp(idstring, "CMAP COOL"))
			cmap = CMAP_COOL;
		else if (!strcmp(idstring, "CMAP BONE"))
			cmap = CMAP_BONE;
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->colormap != cmap))
		{
			plotdata->colormap = cmap;
			(*((colormap[plotdata->colormap]).cmap))(XtDisplay(w), plotdata->ncolors, plotdata->colors, NULL);
			spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MEM npoles", 10))
	{
		sscanf(idstring, "MEM npoles %d", &npoles);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->npoles != npoles))
		{
			plotdata->npoles = npoles;
			defaults.npoles = npoles;
			if (plotdata->method == METHOD_MAXENTROPY)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MTM nwin", 8))
	{
		sscanf(idstring, "MTM nwin %d", &nwin);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->nwin != nwin))
		{
			plotdata->nwin = nwin;
			if (plotdata->method == METHOD_MULTITAPER)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MTM npi", 7))
	{
		sscanf(idstring, "MTM npi %d", &npi);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->npi != npi))
		{
			plotdata->npi = npi;
			if (plotdata->method == METHOD_MULTITAPER)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MTM inoise", 10))
	{
		sscanf(idstring, "MTM inoise %d", &inoise);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->inoise != inoise))
		{
			plotdata->inoise = inoise;
			if (plotdata->method == METHOD_MULTITAPER)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MTM ilog", 8))
	{
		sscanf(idstring, "MTM ilog %d", &ilog);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->ilog != ilog))
		{
			plotdata->ilog = ilog;
			if (plotdata->method == METHOD_MULTITAPER)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MTM itype", 9))
	{
		sscanf(idstring, "MTM itype %d", &itype);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->itype != itype))
		{
			plotdata->itype = itype;
			if (plotdata->method == METHOD_MULTITAPER)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MTM ithresh", 11))
	{
		sscanf(idstring, "MTM ithresh %d", &ithresh);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->ithresh != ithresh))
		{
			plotdata->ithresh = ithresh;
			if (plotdata->method == METHOD_MULTITAPER)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "MTM isignal", 11))
	{
		sscanf(idstring, "MTM isignal %d", &isignal);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plotdata->isignal != isignal))
		{
			plotdata->isignal = isignal;
			if (plotdata->method == METHOD_MULTITAPER)
				spectrum_display(plot);
		}
	}
	else if (!strncmp(idstring, "FFT olap", 8))
	{
		sscanf(idstring, "FFT olap %d", &fftolap);
		XtVaGetValues(w, XmNset, &value, NULL);
		if ((value == True) && (plot->group->fftolap != fftolap))
		{
			plot->group->fftolap = fftolap;
			defaults.fftolap = fftolap;
			spectrum_display(plot);
		}
	}
	else if (!strcmp(idstring, "Save"))
	{
		spectrum_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		spectrum_print(plot, NULL, NULL);
	}
}


/*
** For the following, I employ the following notation:
** X, Y refer to pixel coordinates.
** TIME, VAL are in milliseconds and y units, respectively.
** SAMPLE is time in samples
*/
float spectrum_conv_pixel_to_time(PLOT *plot, int x)
{
	return ((((float)(x - plot->offx) / (float)(plot->width)) * 
		(float)(plot->group->xrangemax - plot->group->xrangemin)) + (float)plot->group->xrangemin);
}

float spectrum_conv_pixel_to_yval(PLOT *plot, int y)
{
	return (((float)plot->group->frmax - (((float)(y - plot->offy) / \
		(float)(plot->height)) * (float)(plot->group->frmax - plot->group->frmin))));
}


#define IMAGESIZE(xi) ( ((xi)->format != ZPixmap) ? ((xi)->bytes_per_line * (xi)->height * (xi)->depth) : ((long)((xi)->bytes_per_line * (xi)->height)) )
/*
** Draws a sonograph into a pixmap
**
** dpy                                   == needed for XDrawSegments
** pixmap                                == must be already created
** gc                                    == for drawing the line segments
** colors, ncolors                       == array of color map indices
** depth                                 == for the XCreateImage call
** offx, offy, width, height             == defines a rectangle in which to draw
** sono, ntimebins, nfreqbins            == the sonogram and its dimensions
** xmin, xmax, fmin, fmax, gsmin, gsmax  == the time axis bounds (in samples) and the gray range
*/
static void DrawSpectrum(Display *dpy, Pixmap pixmap, GC gc, unsigned long *colors, int ncolors, int depth,
	int offx, int offy, int width, int height,
	struct sonogram *sono, short *samples, int nsamples,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float gsmin, float gsmax, float fmin, float fmax)
{
	XImage *image;
	int tmin, tmax;
	int colorindex;
	float *spec_col;
	int x, y, t, nfreqbins, ntimebins, fftsize, overlap;
	float multfactor, tres;

	/*
	** The last check here (height > 65000) is a workaround for a Motif bug.
	** When I select 2 files and draw a spectrum for each, then the second one
	** gets a size of 65518 - which is presumably an error
	*/
	if ((sono == NULL) || (xmin == xmax) || (height < 10) || (height > 65000))
		return;

	nfreqbins = sono->nfreqbins;
	ntimebins = sono->ntimebins;
	fftsize = sono->fftsize;
	overlap = sono->overlap;

	/*
	** Allocate the image
	*/
	image = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)), depth, ZPixmap, 0, NULL,
		1, height, XBitmapPad(dpy), 0);
	image->data = (char *)malloc(IMAGESIZE(image));

	/*
	** Calculate a bunch of constants that will speed up the loops below.
	** Convert x (in samples) to t (in timebins) (depends on overlap and fftsize)
	*/
	tmin = xmin / (((100.0 - overlap) / 100.0) * fftsize);
	tmax = xmax / (((100.0 - overlap) / 100.0) * fftsize);
	if (tmax >= ntimebins) tmax = ntimebins - 1;
	tres = (float)(tmax - tmin) / width;
	multfactor = (ncolors - 1) / (gsmax - gsmin);

	sonogram_setopts(sono, SONO_OPT_NFREQBINS, height);
	cursor_set_busy(toplevel);
	for (x=0; x < width; x++)
	{
		t = tmin + (int)(x * tres);
		spec_col = calculate_psd_cached_column(sono, samples, nsamples, t, fmin, fmax);
		for (y=0; y < height; y++)
		{
			colorindex = (spec_col[y] - gsmin) * multfactor;
			if (colorindex < 0) colorindex = 0;
			else if (colorindex > ncolors-1) colorindex = ncolors - 1;
			XPutPixel(image, 0, height - y - 1, colors[colorindex]);
		}
		XPutImage(dpy, pixmap, gc, image, 0, 0, offx + x, offy, 1, height);
	}
	cursor_unset_busy(toplevel);
	XDestroyImage(image);
}


static void PSDrawSpectrum(FILE *fp, int ncolors,
	int offx, int offy, int offx2, int offy2, int width, int height,
	struct sonogram *sono, short *samples, int nsamples,
	float xrangemin, float xrangemax,
	int xmin, int xmax, float gsmin, float gsmax, float fmin, float fmax)
{
	int t, tmin, tmax;
	int colorindex;
	float *spec_col;
	int x, y;
	float multfactor;
	int nfreqbins, ntimebins, fftsize, overlap;
	int buf[4], j;
	unsigned long word;
	int dpi = 300;

	if ((sono == NULL) || (xmin == xmax) || (height < 10))
		return;

	nfreqbins = sono->nfreqbins;
	ntimebins = sono->ntimebins;
	fftsize = sono->fftsize;
	overlap = sono->overlap;

	/*
	** Calculate any FFTs that still need calculation.
	*/
	if (sono->method == METHOD_FFT)
	{
		tmin = xmin / (((100.0 - overlap) / 100.0) * fftsize);
		tmax = xmax / (((100.0 - overlap) / 100.0) * fftsize);
		if (tmax >= ntimebins) tmax = ntimebins - 1;
		multfactor = (ncolors - 1) / (gsmax - gsmin);

		fprintf(fp, "gsave\n");
		fprintf(fp, "%d %d translate\n", offx, offy2);
		fprintf(fp, "%d %d scale\n", width, height);
		fprintf(fp, "/hexstr %d string def\n", nfreqbins);
		fprintf(fp, "/dotimage {\n");
		fprintf(fp, "    %d %d 8\n", tmax - tmin + 1, nfreqbins);
		fprintf(fp, "    [%d %d %d %d %d %d]\n", tmax - tmin + 1, 0, 0, nfreqbins, 0, 0);
		fprintf(fp, "    {currentfile hexstr readhexstring pop}\n");
		fprintf(fp, "    image\n");
		fprintf(fp, "} def\n");
		fprintf(fp, "dotimage\n");

		for (t=tmin; t <= tmax; t++)
			if (sono->cachetimebins[t] == NULL)
				calculate_psd_cached_column(sono, samples, nsamples, t, fmin, fmax);

		for (y=0; y < nfreqbins; y++)
		{
			for (t=tmin; t <= tmax; t++)
			{
				spec_col = sono->cachetimebins[t] + y;
				colorindex = (*spec_col - gsmin) * multfactor;
				if (colorindex < 0) colorindex = 0;
				else if (colorindex > ncolors-1) colorindex = ncolors - 1;
				colorindex = ncolors - 1 - colorindex;
				fprintf(fp, "%02x", colorindex);
				if ((t != tmin) && ( ((t-tmin) % 128) == 0 )) fprintf(fp, "\n");
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "grestore\n");
	}
	else if (sono->method == METHOD_MAXENTROPY)
	{
		fprintf(fp, "gsave\n");
		fprintf(fp, "%d %d translate\n", offx, offy2);
		fprintf(fp, "%d %d scale\n", width, height);
		width *= (dpi / 72.0);
		height *= (dpi / 72.0);
		fprintf(fp, "/hexstr %d string def\n", height);
		fprintf(fp, "/dotimage {\n");
		fprintf(fp, "    %d %d 8\n", height, width);
		fprintf(fp, "    [%d %d %d %d %d %d]\n", 0, width, height, 0, 0, 0);
		fprintf(fp, "    currentfile /ASCII85Decode filter\n");
		fprintf(fp, "    image\n");
		fprintf(fp, "} def\n");
		fprintf(fp, "dotimage\n");

		multfactor = (ncolors - 1) / (gsmax - gsmin);
		j = 0;
		if ((spec_col = (float *)malloc(height * sizeof(float))) != NULL)
		{
			for (x=0; x < width; x++)
			{
				calculate_psd(sono, samples, nsamples, xmin + x * ((xmax - xmin) / (width-1.0)), height, spec_col, 0, 0.5);
				fprintf(stderr, "%d of %d\n", x, width);
				for (y=0; y < height; y++)
				{
					colorindex = (spec_col[y] - gsmin) * multfactor;
					if (colorindex < 0) colorindex = 0;
					else if (colorindex > ncolors-1) colorindex = ncolors - 1;
					colorindex = ncolors - 1 - colorindex;
					buf[j%4] = colorindex;
					if (((++j)%4) == 0)
					{
						word = ((unsigned long)(((unsigned int)buf[0] << 8) + buf[1]) << 16) + (((unsigned int)buf[2] << 8) + buf[3]);
						if (word == 0) fputc('z', fp); else ASCII85_output(fp, word, 4);
					}
					if ((j % 128) == 0) fputc('\n', fp);
				}
			}
			free(spec_col);
		}
		if ((j%4) != 0)
		{
			int i;

			for (word=0, i=(j%4) - 1; i >=0; i--)
				word += (unsigned long)buf[i] << 8 * (3-i);
			ASCII85_output(fp, word, j%4);
		}
		fputc('~', fp);
		fputc('>', fp);
		fprintf(fp, "grestore\n");
	}
}

static void ASCII85_output(FILE *fp, int unsigned long word, int count)
{
	int i;
	unsigned long v;
	const static unsigned long power85[5] = { 1L, 85L, 85L*85, 85L*85*85, 85L*85*85*85};

	for (i=4; i >= 4-count; i--)
	{
		v = word / power85[i];
		fputc((unsigned char)v + '!', fp);
		word -= v * power85[i];
	}
}

