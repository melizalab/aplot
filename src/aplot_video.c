
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
struct videodata
{
	GC drawing_GC, inverse_GC, mark_GC;
	Pixmap pixmap;
	XImage *image;
	Boolean pixmapalloced;
	Boolean butgrabbed;

	float min, max;
	int ncolors;
	Colormap cmap;
	unsigned long colors[MAXCOLORS];
	XCC xcc;
	int colormap;												 /* Type of colormap */

	int nframes;
	int width;
	int height;
	int ncomps;
	int microsecs_per_frame;
	char *framedata;
	int framenum;
	int loadedframenum;
};


/**********************************************************************************
 * DEFINES
 **********************************************************************************/
#define SONO_DEFAULT_MAX_COLORS 64
#define MAXCOLORS 256
#define RESERVED_COLORS 32
#define BRIGHTEST 65535

#define CreateMenuToggle(widget, parent, name, label, set, userdata) \
	widget = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNset, (set), 															\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNvalueChangedCallback, video_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


#define CreateMenuButton(widget, parent, name, label, userdata) \
	widget = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,	\
		XmNlabelString, xmstr1 = XmStringCreateSimple(label),					\
		XmNuserData, userdata,													\
		NULL);																	\
	XtAddCallback(widget, XmNactivateCallback, video_popupmenu_cb, (XtPointer)plot);	\
	XmStringFree(xmstr1);


/**********************************************************************************
 * PROTOTYPES
 **********************************************************************************/
int video_open(PLOT *plot);
int video_display(PLOT *plot);
int video_set(PLOT *plot);
int video_close(PLOT *plot);
int video_print(PLOT *plot, FILE *fp, char **ret_filename_p);
int video_save(PLOT *plot);
int video_play(PLOT *plot, int reqoffset, int reqsize, short **retsamples, int *retsize, int *playtime);
void video_showvideoframe(PLOT *plot, unsigned long long ntime);
static void video_expose_callback(Widget w, XtPointer clientData, XtPointer callData);
static void video_resize_callback(Widget w, XtPointer clientData, XtPointer callData);
static void video_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b);
static void video_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData);


/**********************************************************************************
 * FUNCTIONS
 **********************************************************************************/
int video_open(PLOT *plot)
{
	struct videodata *plotdata;
	Display *dpy;
	Screen *scr;
	Widget w, mw, m0, m1, m2, m3;
	int status = SUCCESS;
	Dimension width, height;
	int depth;
    XGCValues values;
	unsigned long foreground, background;
	XmString xmstr, xmstr1;
	Arg args[10];
	int n;

	plot->plotdata = (void *)calloc(1, sizeof(struct videodata));
	plotdata = (struct videodata *)plot->plotdata;
	plotdata->image = (XImage *)calloc(sizeof(XImage), 1);
	plotdata->framedata = NULL;
	plotdata->width = 0;
	plotdata->height = 0;
	plotdata->ncomps = 0;
	plotdata->microsecs_per_frame = 0;
	plotdata->colormap = defaults.colormap;
	plotdata->framenum = 0;
	plotdata->loadedframenum = -1;

	plot->plot_widget = XtVaCreateManagedWidget("video", xmDrawingAreaWidgetClass, plot->panel->panel_container,
		XmNheight, defaults.video_height,
		XmNwidth, defaults.width,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);
	XtAddCallback(plot->plot_widget, XmNexposeCallback, video_expose_callback, (XtPointer)plot);
	XtAddCallback(plot->plot_widget, XmNresizeCallback, video_resize_callback, (XtPointer)plot);

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

	plot->offx = 0;
	plot->offy = 0;
	plot->offx2 = 0;
	plot->offy2 = 0;
	plot->width = width;
	plot->height = height;
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
	** drawing_GC is for the picture itself.  inverse_GC is for erasing.  mark_GC is for the subregion marks.
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

	plotdata->pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), width, height, depth);
	plotdata->pixmapalloced = TRUE;
	XFillRectangle(dpy, plotdata->pixmap, plotdata->inverse_GC, 0, 0, width, height);

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

	CreateMenuButton(m2, mw, "save", "Save", "Save");
	CreateMenuButton(m3, mw, "print", "Print EPS", "Print EPS");

	/*
	** Register an event handler
	*/
	XtAddEventHandler(w, KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | Button2MotionMask,
		FALSE, (XtEventHandler) video_event_handler, (XtPointer)plot);

	plot->plot_display = video_display;
	plot->plot_set = video_set;
	plot->plot_close = video_close;
	plot->plot_print = video_print;

	plot->plot_playmarker = NULL;
	plot->plot_showvideoframe = video_showvideoframe;
	plot->plot_clearmarks = NULL;
	plot->plot_drawstartmark = NULL;
	plot->plot_drawstopmark = NULL;
	plot->plot_conv_pixel_to_time = NULL;

	plot->plot_save = video_save;

	plot->plot_event_handler = video_event_handler;
	plot->plot_play = video_play;

	plotdata->butgrabbed = FALSE;
	plot->playmark = -1;
	plot->markx1 = -1;
	plot->markx2 = -1;

	plot->group->needpcm = 1;

	return status;
}

int video_display(PLOT *plot)
{
	struct videodata *plotdata = (struct videodata *)plot->plotdata;
	int status = SUCCESS, tstatus;
	GROUP *group = plot->group;

	/*
	** Load the video data, but only if we need to...
	*/
	if (group->viddirty == 1)
	{
		if (load_viddata(&group->vidfp, group->loadedfilename, group->filename, group->entry, 
			&plotdata->nframes, &plotdata->width, &plotdata->height, &plotdata->ncomps,
			&plotdata->microsecs_per_frame,
			&group->ntime) == 0)
		{
			if (plotdata->framedata)
			{
				free(plotdata->framedata);
				plotdata->framedata = (char *)calloc(plotdata->height * plotdata->width, plotdata->ncomps);
			}
			group->viddirty = 0;
			plotdata->image->width = plotdata->width;
			plotdata->image->height = plotdata->height;
			plotdata->image->xoffset = 0;
			plotdata->image->format = ZPixmap; /* XYBitmap, XYPixmap, or ZPixmap */
			plotdata->image->data = NULL;
			plotdata->image->byte_order = LSBFirst;
			plotdata->image->bitmap_unit = 8;
			plotdata->image->bitmap_bit_order = LSBFirst;
			plotdata->image->bitmap_pad = 8;
			plotdata->image->depth = plotdata->ncomps;
			plotdata->image->bytes_per_line = plotdata->width * plotdata->ncomps;
			plotdata->image->bits_per_pixel = plotdata->ncomps * 8;
			plotdata->image->red_mask = 0;
			plotdata->image->green_mask = 0;
			plotdata->image->blue_mask = 0;
			XInitImage(plotdata->image);
			plotdata->image->data = plotdata->framedata;
		}
		else status = ERROR;
	}

	/*
	** Update the plot
	*/
	tstatus = video_set(plot);
	return (status == SUCCESS) ? (tstatus) : (status);
}

void video_showvideoframe(PLOT *plot, unsigned long long ntime)
{
	struct videodata *plotdata = (struct videodata *)plot->plotdata;
	GROUP *group = plot->group;
	VIDCLOSESTFRAME vcf;

	vcf.ntime = ntime;
	vcf.framenum = -1;
	vid_ctl(group->vidfp, VIDIOGETCLOSESTFRAME, &vcf);
	plotdata->framenum = vcf.framenum;
	video_set(plot);
	return;
}

int video_set(PLOT *plot)
{
	struct videodata *plotdata = (struct videodata *)plot->plotdata;
	GROUP *group = plot->group;
	Display *dpy = XtDisplay(plot->plot_widget);
	Widget w = (Widget)plot->plot_widget;
	Dimension height, width;
	Window root_return;
	int x_return, y_return;
	unsigned int pixwidth, pixheight, border_width_return, depth_return;
	unsigned long long ntime;
	int haschanged = 0;

	/*
	** We do this here because the window must be realized to grab the mouse
	** it isn't realized during the call to video_open().
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
	** Retrieve the window size
	**
	** The check here for height > 65000 is a workaround for a Motif bug.
	** When I select 2 files and draw a video for each, then the second one
	** gets a size of 65518 - which is presumably an error
	*/
	XtVaGetValues(w, XmNheight, &height, XmNwidth, &width, NULL);
	if ((height > 65000) || (height < 10))
		return SUCCESS;
	plot->width = width;
	plot->height = height;

	/*
	** If the window size has changed, we need to reallocate the
	** pixmap.
	*/
	XGetGeometry(dpy, plotdata->pixmap, &root_return, &x_return, &y_return,
		&pixwidth, &pixheight, &border_width_return, &depth_return);
	if ((pixheight != height) || (pixwidth != width))
	{
		if (plotdata->pixmapalloced)
			XFreePixmap(dpy, plotdata->pixmap);
		plotdata->pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), width, height, plot->depth);
		plotdata->pixmapalloced = TRUE;
		haschanged++;
	}
	XFillRectangle(dpy, plotdata->pixmap, plotdata->inverse_GC, 0, 0, width, height);

	/*
	** Clear any marks
	*/
	plot->playmark = -1;
	plot->markx1 = -1;
	plot->markx2 = -1;

	/*
	** Load the desired frame
	*/
	if (plotdata->framenum != plotdata->loadedframenum)
	{
		if (vid_read(group->vidfp, plotdata->framenum, plotdata->framedata, &ntime) != 0)
		{
			fprintf(stderr, "Error reading frame #%d for file '%s'\n", plotdata->framenum, group->loadedfilename);
		}
		plotdata->loadedframenum = plotdata->framenum;
		haschanged++;
	}

	/*
	** Do the rendering to an off-screen pixmap, then copy to the window.
	*/
	if (haschanged)
	{
		cursor_set_busy(toplevel);
		XPutImage(dpy, plotdata->pixmap, plotdata->drawing_GC, plotdata->image, 0, 0, 0, 0, width, height);
		XCopyArea(dpy, plotdata->pixmap, XtWindow(w), plotdata->drawing_GC, 0, 0, width, height, 0, 0);
		cursor_unset_busy(toplevel);
	}
	return SUCCESS;
}

int video_close(PLOT *plot)
{
	struct videodata *plotdata = (struct videodata *)plot->plotdata;
	Widget w = (Widget)plot->plot_widget;

	if (plot->ticklblfont != NULL) XmFontListFree(plot->ticklblfont);
	if (plotdata->pixmapalloced) XFreePixmap(XtDisplay(w), plotdata->pixmap);
	if (plotdata->xcc) XCCFree(plotdata->xcc);
	if (plotdata->framedata) free(plotdata->framedata);
	XtReleaseGC(w, plotdata->drawing_GC);
	XtReleaseGC(w, plotdata->inverse_GC);
	XtReleaseGC(w, plotdata->mark_GC);
	XtDestroyWidget(plot->plot_widget);
	free(plotdata->image);
	free(plot->plotdata);
	plot->plotdata = NULL;
	return SUCCESS;
}

int video_print(PLOT *plot, FILE *fp, char **ret_filename_p)
{
	return SUCCESS;
}

int video_save(PLOT *plot)
{
 	return SUCCESS;
}

int video_play(PLOT *plot, int reqoffset, int reqsize, short **retsamples, int *retsize, int *playtime)
{
	return SUCCESS;
}

static void video_expose_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;
	PLOT *plot = (PLOT *)clientData;
	struct videodata *plotdata = (struct videodata *)plot->plotdata;
	XExposeEvent *expevent;

	if (cbs != NULL)
	{
		expevent = (XExposeEvent *)cbs->event;
		if (plotdata->pixmapalloced)
		{
			XCopyArea(XtDisplay(w), plotdata->pixmap, XtWindow(w), plotdata->drawing_GC,
				expevent->x, expevent->y, expevent->width, expevent->height, expevent->x, expevent->y);
		}
	}
}

static void video_resize_callback(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;

	video_set(plot);
	return;
}


/*
**  Button1 press: evaluate at curpos
**  Button2 press: clear marks.  start mark at curpos.
**  Button2 motion: clear old stop mark.  stop mark at curpos
**  Button2 release: same as motion.
*/
static void video_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
{
	PLOT *plot = (PLOT *)clientData;
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
					if (KEY_IS_UP(keybuf))
					{
						panel_unzoom(plot->panel);
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
			butevent = (XButtonEvent *) event;
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
			if (motevent->state & Button1Mask)
			{
			}
			else if (motevent->state & Button2Mask)
			{
			}
			else if (motevent->state & Button2Mask)
			{
			}
			break;

		default:
			break;
	}
}


static void video_popupmenu_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	PLOT *plot = (PLOT *)clientData;
	char *idstring;

	XtVaGetValues(w, XmNuserData, &idstring, NULL);
	if (!strcmp(idstring, "Save"))
	{
		video_save(plot);
	}
	else if (!strcmp(idstring, "Print EPS"))
	{
		video_print(plot, NULL, NULL);
	}
}


