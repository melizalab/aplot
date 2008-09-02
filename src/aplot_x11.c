
/**********************************************************************************
 * INCLUDES
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <Xm/Xm.h>

void DrawXAxis(Display *dpy, Pixmap pixmap, GC gc, XmFontList ticklblfont, int offx, int offy, int width, int height, float x0, float x1);
void DrawYAxis(Display *dpy, Pixmap pixmap, GC gc, XmFontList ticklblfont, int offx, int offy, int width, int height, float y0, float y1);
void DrawXGrid(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height, float x0, float x1);
void DrawYGrid(Display *dpy, Pixmap pixmap, GC gc, int offx, int offy, int width, int height);
XFontStruct *getFontStruct(XmFontList font);

int snd_gen_axis(XmFontList ticklblfont, int npixels, int vertflag, float n1, float n2, float *inc, float *start, float *stop);

static double dbl_raise(double x, int y);
static double make_tics(double tmin, double tmax);

#define CONV_VALUE_TO_Y(y0, y1, height, offy, value) (offy + height - (((value - y0) / (y1 - y0)) * (height)))
#define CONV_TIME_TO_X(time, x0, x1, width, offx) (((width) * (((time) - (x0)) / ((x1) - (x0)))) + offx)

/*
** draws the X axis, with tick marks and labels
**
** dpy									== needed for XDrawLine, XStringDraw
** pixmap								== must be already created
** gc									== for drawing the line segments and text
** ticklblfont 							== for the tick labels
** offx,offy,height,width				== defines where to draw
** x0,x1								== the axis bounds (in milliseconds)
*/
void DrawXAxis(Display *dpy, Pixmap pixmap, GC gc, XmFontList ticklblfont,
	int offx, int offy, int width, int height, float x0, float x1)
{
	int ticpixel;
	float inc, start, stop, tic;
	char buf[20];
	XmString xmstr;

	/*
	** Draw X axis
	*/
	XDrawLine(dpy, pixmap, gc, offx, offy + height, width + offx, offy + height);

	/*
	** Compute where to put the tick marks.
	** First, convert xmin and xmax into milliseconds.
	** The result of this computation will be a {start, inc, stop}
	*/
	if (x0 >= x1)
		return;
	inc = make_tics(x0, x1);
	start = inc * floor(x0/inc);
	stop = inc * ceil(x1/inc);

	snd_gen_axis(ticklblfont, width, 0, x0, x1, &inc, &start, &stop);

	/*
	** Convert {start, stop, inc} from time to pixel coordinates.
	** Then draw the tick mark and the tick label.
	*/
	for (tic = start; tic <= stop; tic += inc)
	{
		if ((tic < x0) || (tic > x1))
			continue;
		ticpixel = CONV_TIME_TO_X(tic, x0, x1, width, offx);
		XDrawLine(dpy, pixmap, gc, ticpixel, offy + height - 3, ticpixel, offy + height + 3);
		sprintf(buf, "%g", tic);
		xmstr = XmStringCreateSimple(buf);
		XmStringDraw(dpy, pixmap, ticklblfont, xmstr, gc,
			ticpixel - 0.5 * XmStringWidth(ticklblfont, xmstr), offy + height + 3, 100, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
		XmStringFree(xmstr);
	}
}


/*
** draws the Y axis, with tick marks and labels
**
** dpy									== needed for XDrawLine, XStringDraw
** pixmap								== must be already created
** gc									== for drawing the line segments and text
** ticklblfont 							== for the tick labels
** offx,offy,offx2,offy2,height,width	== defines where to draw
** xmin,xmax,ymin,ymax					== the axis bounds (in samples)
*/
void DrawYAxis(Display *dpy, Pixmap pixmap, GC gc, XmFontList ticklblfont,
	int offx, int offy, int width, int height, float y0, float y1)
{
	int ticpixel;
	float inc, start, stop, tic;
	char buf[20];
	XmString xmstr;

	/*
	** Draw Y axis
	*/
	XDrawLine(dpy, pixmap, gc, offx, offy, offx, offy + height);

	/*
	** Compute where to put the tick marks.
	** The result of this computation will be a {start, inc, stop}
	*/
	if (y0 >= y1)
		return;
	inc = make_tics(y0, y1);
	start = inc * floor(y0/inc);
	stop = inc * ceil(y1/inc);
	while (start < y0) start += inc;
	while (stop > y1) stop -= inc;

	snd_gen_axis(ticklblfont, height, 1, y0, y1, &inc, &start, &stop);

	/*
	** Convert {start, stop, inc} from value to pixel coordinates.
	** Then draw the tick mark and the tick label.
	*/
	for (tic = start; tic <= stop; tic += inc)
	{
		if ((tic < y0) || (tic > y1))
			continue;
		ticpixel = CONV_VALUE_TO_Y(y0, y1, height, offy, tic);
		XDrawLine(dpy, pixmap, gc, offx - 3, ticpixel, offx + 3, ticpixel);
		sprintf(buf, "%g", tic);
		xmstr = XmStringCreateSimple(buf);
		XmStringDraw(dpy, pixmap, ticklblfont, xmstr, gc,
			0, ticpixel - 0.5 * XmStringHeight(ticklblfont, xmstr), offx - 4, XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);
		XmStringFree(xmstr);
	}
}


/*
** draws the X grid, with tick marks every 200ms (meant to simulate a Kay Sonagraph)
**
** dpy									== needed for XDrawLine, XStringDraw
** pixmap								== must be already created
** gc									== for drawing the line segments and text
** offx,offy,height,width				== defines where to draw
** x0,x1							    == the axis bounds (in milliseconds)
*/
void DrawXGrid(Display *dpy, Pixmap pixmap, GC gc,
	int offx, int offy, int width, int height, float x0, float x1)
{
	int x;
	int ylim = height / 20;
	float x_axis_fstep = (0.2 * ((float)width / ((x1 - x0) / 1000.0)));
	int x_axis_step = x_axis_fstep;
	float x_resid = x_axis_fstep - x_axis_step;
	float resid = 0.0;


	if (x_axis_step > 1)
	{
		for (x=0; x<width; x+=x_axis_step)
		{
			resid += x_resid;
			if (resid >= 1.0)
			{
				x++;
				resid -= 1.0;
			}
			XDrawLine(dpy, pixmap, gc, offx + x, offy + ylim/5, offx + x, offy + ylim);
		}
	}
}


/*
** draws the Y grid, with 10 evenly spaced grid lines 
**
** dpy									== needed for XDrawLine, XStringDraw
** pixmap								== must be already created
** gc									== for drawing the line segments and text
** offx,offy,height,width				== defines where to draw
*/
void DrawYGrid(Display *dpy, Pixmap pixmap, GC gc,
	int offx, int offy, int width, int height)
{
	int y;
	float y_axis_fstep = ((float)height) / 10.0;
	int y_axis_step = y_axis_fstep;
	float y_resid = y_axis_fstep - y_axis_step;
	float resid = 0.0;

	for (y=height - 1; y>=0; y -= y_axis_step)
	{
		resid += y_resid;
		if (resid >= 1.0)
		{
			y--;
			resid -= 1.0;
		}
		XDrawLine(dpy, pixmap, gc, offx, offy + y, offx + width, offy + y);
	}
}


/*
** Get the XFontStruct that corresponds to the default (first) font in
** a Motif font list.  Since Motif stores this, it saves us from storing
** it or querying it from the X server.
*/
XFontStruct *getFontStruct(XmFontList font)
{
    XFontStruct *fs;
    XmFontContext context;
    XmStringCharSet charset;

    XmFontListInitFontContext(&context, font);
    XmFontListGetNextFont(context, &charset, &fs);
    XmFontListFreeFontContext(context);
    XtFree(charset);
    return fs;
}


static double dbl_raise(double x, int y)
{
	double val;
	int i;

	val = 1.0;
	for (i=0; i < abs(y); i++)
		val *= x;
	if (y < 0)
		return (1.0/val);
	return(val);
}


static double make_tics(double tmin, double tmax)
{
	double xr,xnorm,tics,tic,l10;

	xr = fabs(tmin-tmax);
	l10 = log10(xr);

	xnorm = pow(10.0,l10-(double)((l10 >= 0.0 ) ? (int)l10 : ((int)l10-1)));
	if (xnorm <= 3)
		tics = 0.3;
	else if (xnorm <= 5)
		tics = 0.5;
	else tics = 1.0;

	tic = tics * dbl_raise(10.0,(l10 >= 0.0 ) ? (int)l10 : ((int)l10-1));
	return(tic);
}

typedef struct
{
	float hi, lo;
	int max_ticks;
	float flo, fhi, mlo, mhi, step, tenstep;
	int tens, min_label_width, max_label_width;
	int min_label_x, min_label_y, max_label_x, max_label_y, maj_tick_len, min_tick_len;
	char *min_label, *max_label;
} tick_descriptor; 

static tick_descriptor *describe_ticks(tick_descriptor *gd_td, float lo, float hi, int max_ticks);

int snd_gen_axis(XmFontList ticklblfont, int npixels, int vertflag, float n1, float n2, float *inc, float *start, float *stop)
{
	tick_descriptor *td;
	char buf[20];
	XmString xmstr;
	int max_ticks;

	/*
	** Figure out how many tick labels will fit given our width
	*/ 
	sprintf(buf, "5%g", n2);
	xmstr = XmStringCreateSimple(buf);
	if (vertflag == 0)
		max_ticks = floor(npixels / (1.1 * XmStringWidth(ticklblfont, xmstr)));
	else
		max_ticks = floor(npixels / (1.1 * XmStringHeight(ticklblfont, xmstr)));

	td = describe_ticks(NULL, n1, n2, max_ticks);
#ifdef notdef
	fprintf(stderr, "Called describe_ticks with %.2f, %.2f, %d\n", n1, n2, max_ticks);
	fprintf(stderr, "flo = %.2f\n", td->flo);
	fprintf(stderr, "fhi = %.2f\n", td->fhi);
	fprintf(stderr, "mlo = %.2f\n", td->mlo);
	fprintf(stderr, "mhi = %.2f\n", td->mhi);
	fprintf(stderr, "step = %.2f\n", td->step);
	fprintf(stderr, "tenstep = %.2f\n", td->tenstep);
	fprintf(stderr, "tens = %d\n", td->tens);
#endif

	*inc = td->step;
	*start = td->flo;
	*stop = td->fhi;
	free(td);
	return 0;
}

/*
** given absolute (unchangeable) axis bounds lo and hi, and abolute maximum number
** of ticks to use, find a "pretty" tick placement much of the work here involves
** floating point rounding problems.  We assume the tick labeller will round as well
*/
static tick_descriptor *describe_ticks(tick_descriptor *gd_td, float lo, float hi, int max_ticks)
{
	tick_descriptor *td;

	int ten,hib,lob;
	double flog10,plog10;
	float frac,ften,hilo_diff,eten,flt_ten,flt_ften;

	float inside,mfdiv,mten,mften;
	int mticks,mdiv;

	if (!gd_td)
		td = (tick_descriptor *)calloc(1,sizeof(tick_descriptor));
	else
	{
		td = gd_td;
		if ((td->hi == hi) && (td->lo == lo) && (td->max_ticks == max_ticks)) return(td);
	}
	td->hi = hi;
	td->lo = lo;
	td->max_ticks = max_ticks;
	hilo_diff = hi-lo;
	flt_ten=log10(hilo_diff);
	ten=(int)floor(flt_ten);
	frac=flt_ten-ten;
	if (frac>.9999) ten++;
	eten=pow(10,ten);
	hib=(int)floor(hi/eten);
	lob=(int)ceil(lo/eten);
	if (lob != hib)
	{
		td->mlo=(float)(lob*eten);
		td->mhi=(float)(hib*eten);
	}
	else
	{
		/* try next lower power of ten */
		ften = eten*.1;
		flt_ften=(hi/ften);
		hib=(int)floor(flt_ften);
		frac=flt_ften-hib;
		if (frac > .9999) hib++;
		lob=(int)ceil(lo/ften);
		td->mlo=(float)(lob*ften);
		td->mhi=(float)(hib*ften);
	}
	inside = (td->mhi-td->mlo)/hilo_diff;
	mticks = (int)floor(inside*max_ticks);
	if (mticks < 1) mdiv=1;
	else if (mticks < 3) mdiv=mticks;
	else if (mticks == 3) mdiv=2;
	else if (mticks < 6) mdiv=mticks;
	else if (mticks < 10) mdiv=5;
	else mdiv = (int)(10*floor(mticks/10));
	mfdiv = (td->mhi-td->mlo)/mdiv;
	flog10 = floor(log10(mfdiv));
	plog10 = pow(10,flog10);
	td->tens = (int)fabs(flog10);
	mten = (float)(floor(4.0*(.00001+(mfdiv/plog10))))/4.0;
	if (mten < 1.0) mten = 1.0;
	if ((mten == 1.0) || (mten == 2.0) || (mten == 2.5) || (mten == 5.0)) ften = mten;
	else if (mten < 2.0) ften=2.0;
	else if (mten < 2.5) ften=2.5;
	else if (mten < 5.0) ften=5.0;
	else ften=10.0;
	td->tenstep = ften;
	mften = ften*plog10;
	td->step = mften;
	flt_ten=lo/mften;
	lob=(int)ceil(flt_ten);
	frac=lob-flt_ten;
	if (frac > .9999) lob--;
	td->flo=lob*mften;
	flt_ten=(hi/mften);
	hib=(int)floor(flt_ten);
	frac=flt_ten-hib;
	if (frac > .9999) hib++;
	td->fhi=hib*mften;
	return(td);
} 

