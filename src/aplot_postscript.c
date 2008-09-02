
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

#define CONV_TIME_TO_X(time, x0, x1, width, offx) (((width) * (((time) - (x0)) / ((x1) - (x0)))) + offx)
#define CONV_VALUE_TO_Y(value, y0, y1, height, offy, offy2) (((height) * (((value) - (y0)) / ((y1) - (y0)))) + offy2)

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

void PSInit(FILE *fp, char *filename, int width, int height);
void PSFinish(FILE *fp);
void PSDrawLine(FILE *fp, int x1, int y1, int x2, int y2);
void PSDrawLineFP(FILE *fp, float x1, float y1, float x2, float y2);
void PSDrawLines(FILE *fp, XPoint *pts, int nsegs);
void PSDrawLinesFP(FILE *fp, PSPoint *pts, int nsegs);
void PSDrawPoints(FILE *fp, XPoint *pts, int nsegs);
void PSDrawPointsFP(FILE *fp, PSPoint *pts, int nsegs);
void PSDrawSpikesFP(FILE *fp, PSPoint *pts, int nsegs);
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
void PSDrawFBoxFP(FILE *fp, float width, float height, float bottom, float left);
void PSSet_Pref(char *property, char *value);

static double dbl_raise(double x, int y);
static double make_tics(double tmin, double tmax);
static int pref_dottype = 0;
static float pref_dashheight = 1.0;

static char *prolog = "\
%% Draw a dot. Args are x, y\n\
/d {\n\
currentlinewidth 2 div sub moveto\n\
0 currentlinewidth rlineto\n\
} def\n\
%% Draw a half-dash. Args are x, y\n\
/e {\n\
0.25 sub moveto\n\
0 0.50 rlineto\n\
} def\n\
%% Draw a dash. Args are x, y\n\
/f {\n\
0.50 sub moveto\n\
0 1.0 rlineto\n\
} def\n\
%% Draw a dash with definable height. Args are x, y\n\
/rowheight 1 def\n\
/g {\n\
0 sub moveto\n\
0 rowheight rlineto\n\
} def\n\
%% Draw a line.  Args are x1, y1, x2, y2\n\
/l {\n\
moveto lineto stroke\n\
} def\n\
%% Draw a rectangle.  Args are width, height, bottom, left\n\
/rect {\n\
gsave\n\
translate\n\
matrix currentmatrix\n\
3 1 roll\n\
scale\n\
newpath\n\
0 0 moveto\n\
0 1 lineto\n\
1 1 lineto\n\
1 0 lineto\n\
closepath\n\
setmatrix\n\
stroke\n\
grestore\n\
} def\n\
%% Draw a filled rectangle.  Args are width, height, bottom, left\n\
/fillrect {\n\
gsave\n\
translate\n\
scale\n\
newpath\n\
0 0 moveto\n\
0 1 lineto\n\
1 1 lineto\n\
1 0 lineto\n\
closepath\n\
fill\n\
grestore\n\
} def\n\
%% Draw an arc.  Args are: x, y, radius, angle1, angle2\n\
/xarc {\n\
newpath\n\
arc\n\
stroke\n\
} def\n\
%% Draw a filled arc.  Args are: x, y, radius, angle1, angle2\n\
/fillarc {\n\
newpath\n\
arc\n\
fill\n\
} def\n\
%% Simple definitions\n\
/rl { rlineto } def\n\
/rm { rmoveto } def\n\
/mt { moveto } def\n\
\n";

static char *parentprolog = "\
/BeginEPSF { %%def\n\
 /b4_Inc_state save def                         %% Save state for cleanup\n\
 /dict_count countdictstack def                 %% Count objects on dict stack\n\
 /op_count count 1 sub def                      %% Count objects on operand stack\n\
 userdict begin                                 %% Push userdict on dict stack\n\
 /showpage { } def                              %% Redefine showpage\n\
 0 setgray 0 setlinecap                         %% overprint to their defaults\n\
 1 setlinewidth 0 setlinejoin\n\
 10 setmiterlimit [ ] 0 setdash newpath\n\
 /languagelevel where                           %% If level not equal to 1 then\n\
 {pop languagelevel                             %% set strokeadjust and\n\
 1 ne                                           %% overprint to their defaults\n\
  {false setstrokeadjust false setoverprint\n\
  } if\n\
 } if\n\
} bind def\n\
/EndEPSF { %%def\n\
 count op_count sub {pop} repeat                %% Clean up stacks\n\
 countdictstack dict_count sub {end} repeat\n\
 b4_Inc_state restore\n\
} bind def\n";


void PSInit(FILE *fp, char *filename, int width, int height)
{
	time_t t;

	/*
	** Write the prolog
	*/
	fprintf(fp, "%%!PS-Adobe-3.0 EPSF-3.0\n");
	fprintf(fp, "%%%%BoundingBox: %d %d %d %d\n", 18, 18, 18 + width, 18 + height);
	fprintf(fp, "%%%%Title: %s\n", filename);
	time(&t);
	fprintf(fp, "%%%%CreationDate: %s\n\n", ctime(&t));

	/*
	** Set up our coordinate system
	*/
	fprintf(fp, "gsave\n");
	fprintf(fp, "18 18 translate\n");
	fprintf(fp, "75 75 div dup scale\n");

	/*
	** Write some routines we'll use commonly
	*/
	fprintf(fp, prolog);

	/*
	** Set up a clipping rectangle
	*/
	fprintf(fp, "0 0 moveto\n");
	fprintf(fp, "%d 0 lineto\n", width);
	fprintf(fp, "%d %d lineto\n", width, height);
	fprintf(fp, "0 %d lineto\n", height);
	fprintf(fp, "closepath clip\n");
	fprintf(fp, "newpath\n");
}

void PSFinish(FILE *fp)
{
	fprintf(fp, "showpage grestore\n");
}

void PSDrawLine(FILE *fp, int x1, int y1, int x2, int y2)
{
	fprintf(fp, "%d %d %d %d l\n", x1, y1, x2, y2);
}

void PSDrawLineFP(FILE *fp, float x1, float y1, float x2, float y2)
{
	fprintf(fp, "%f %f %f %f l\n", x1, y1, x2, y2);
}

void PSDrawLines(FILE *fp, XPoint *pts, int nsegs)
{
	int i;

	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=1; i < nsegs; i++)
		PSDrawLine(fp, pts[i-1].x, pts[i-1].y, pts[i].x, pts[i].y);
}

void PSDrawLinesFP(FILE *fp, PSPoint *pts, int nsegs)
{
	int i, counter;

	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=1, counter = 0; i < nsegs; i++)
	{
		if (counter++ == 0)
		{
			fprintf(fp, "%f %f moveto\n", pts[i-1].x, pts[i-1].y);
		}
		fprintf(fp, "%f %f rl\n", pts[i].x - pts[i-1].x, pts[i].y - pts[i-1].y);
		if (counter == 100)
		{
			counter = 0;
			fprintf(fp, "stroke\n");
		}
	}
	if (counter > 0)
		fprintf(fp, "stroke\n");
}

void PSDrawFBoxFP(FILE *fp, float width, float height, float bottom, float left)
{
	fprintf(fp, "%f %f %f %f fillrect\n", width, height, bottom, left);
}

void PSFillRectangles(FILE *fp, PSRectangle *rects, int nrects)
{
	int i;

	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=0; i < nrects; i++)
	{
		PSDrawFBoxFP(fp, rects[i].width, rects[i].height, rects[i].x, rects[i].y);
	}
}

void PSDrawFBoxesFP(FILE *fp, PSPoint *pts, int nsegs, float width, float y)
{
	int i;

	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=0; i < nsegs; i++)
	{
		PSDrawFBoxFP(fp, width, pts[i].y, pts[i].x, y);
	}
}

void PSDrawPoints(FILE *fp, XPoint *pts, int nsegs)
{
	int i;
	char dottype;

	switch (pref_dottype)
	{
		case 1:	dottype = 'e'; break;
		case 2:	dottype = 'f'; break;
		case 0:
		default: dottype = 'd'; break;
	}
	fprintf(fp, "gsave\n");
	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=0; i < nsegs; i++)
		fprintf(fp, "%d %d %c\n", pts[i].x, pts[i].y, dottype);
	fprintf(fp, "stroke grestore\n");
}

void PSDrawPointsFP(FILE *fp, PSPoint *pts, int nsegs)
{
	int i;
	char dottype;

	switch (pref_dottype)
	{
		case 1:	dottype = 'e'; break;
		case 2:	dottype = 'f'; break;
		case 0:
		default: dottype = 'd'; break;
	}
	fprintf(fp, "gsave\n");
	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=0; i < nsegs; i++)
		fprintf(fp, "%f %f %c\n", pts[i].x, pts[i].y, dottype);
	fprintf(fp, "stroke grestore\n");
}

void PSDrawSpikesFP(FILE *fp, PSPoint *pts, int nsegs)
{
	int i;
	char dottype;

	dottype = 'g';
	fprintf(fp, "gsave\n");
	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=0; i < nsegs; i++)
	{
#ifndef notdef
		fprintf(fp, "%f ", pts[i].x);
		if ((pts[i].y - floor(pts[i].y)) < 0.00001)
			fprintf(fp, "%d ", (int)pts[i].y);
		else
			fprintf(fp, "%f ", pts[i].y);
		fprintf(fp, " %c\n", dottype);
#else
		fprintf(fp, "%f %f %c\n", pts[i].x, pts[i].y, dottype);
#endif
	}
	fprintf(fp, "stroke grestore\n");
}

void PSDrawSegments(FILE *fp, XSegment *segs, int nsegs)
{
	int i;

	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=0; i < nsegs; i++)
		PSDrawLine(fp, segs[i].x1, segs[i].y1, segs[i].x2, segs[i].y2);
}

void PSDrawSegmentsFP(FILE *fp, PSSegment *segs, int nsegs)
{
	int i, counter;

	fprintf(fp, "0.00 0.00 0.00 setrgbcolor 0.50 setlinewidth\n");
	for (i=1, counter = 0; i < nsegs; i++)
	{
		if (counter++ == 0)
		{
			fprintf(fp, "%g %f mt %g %f rl\n", segs[i-1].x1, segs[i-1].y1, segs[i-1].x2 - segs[i-1].x1, segs[i-1].y2 - segs[i-1].y1);
		}
		fprintf(fp, "%g %f rm %g %f rl\n", segs[i].x1 - segs[i-1].x2, segs[i].y1 - segs[i-1].y2, segs[i].x2 - segs[i].x1, segs[i].y2 - segs[i].y1);
		if (counter == 100)
		{
			counter = 0;
			fprintf(fp, "stroke\n");
		}
	}
	if (counter > 0)
		fprintf(fp, "stroke\n");
}

void PSDrawXAxisLabel(FILE *fp, char *string, int x, int y, int width)
{
	fprintf(fp, "newpath\n");
	fprintf(fp, "0.00 0.00 0.00 setrgbcolor /Times-Roman findfont 0010 scalefont setfont\n");
/*	fprintf(fp, "(%s) stringwidth pop 2 div neg %d add %d 13 neg add moveto\n", string, x, y); */
	fprintf(fp, "(%s) stringwidth pop 2 div neg %d add dup 0 lt {pop 0} if dup (%s) stringwidth pop add %d gt {pop %d (%s) stringwidth pop sub} if %d 13 sub moveto\n", string, x, string, width, width, string, y);
	fprintf(fp, "(%s) show\n", string);
}

void PSDrawHorizText(FILE *fp, char *string, float x, float y)
{
	fprintf(fp, "newpath\n");
	fprintf(fp, "0.00 0.00 0.00 setrgbcolor /Times-Roman findfont 0010 scalefont setfont\n");
	fprintf(fp, "%f %f moveto (%s) show\n", x, y, string);
}

void PSDrawYAxisLabel(FILE *fp, char *string, int x, int y)
{
	fprintf(fp, "0.00 0.00 0.00 setrgbcolor /Times-Roman findfont 0010 scalefont setfont\n");
	fprintf(fp, "(%s) stringwidth pop neg %d add 3 neg add %d 2 neg add moveto\n", string, x, y);
	fprintf(fp, "(%s) show\n", string);
}

void PSDrawXAxis(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height, float x0, float x1)
{
	int ticpixel = 0;
	float inc, start, stop, tic;
	char buf[80];

	/*
	** Draw X axis
	*/
	PSDrawLine(fp, offx, offy2, width + offx, offy2);

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

	/*
	** Convert {start, stop, inc} from time to pixel coordinates.
	** Then draw the tick mark and the tick label.
	*/
	for (tic = start; tic <= stop; tic += inc)
	{
		if ((tic < x0) || (tic > x1))
			continue;
		ticpixel = CONV_TIME_TO_X(tic, x0, x1, width, offx);
		PSDrawLine(fp, ticpixel, offy2 + 3, ticpixel, offy2 - 3);
		sprintf(buf, "%g", tic);
		PSDrawXAxisLabel(fp, buf, ticpixel, offy2, offx + width);
	}
}

void PSDrawYAxis(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height, float y0, float y1)
{
	int ticpixel;
	float inc, start, stop, tic;
	char buf[80];

	/*
	** Draw Y axis
	*/
	PSDrawLine(fp, offx, offy2 + height, offx, offy2);

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

	/*
	** Convert {start, stop, inc} from value to pixel coordinates.
	** Then draw the tick mark and the tick label.
	*/
	for (tic = start; tic <= stop; tic += inc)
	{
		if ((tic < y0) || (tic > y1))
			continue;
		ticpixel = CONV_VALUE_TO_Y(tic, y0, y1, height, offy, offy2);
		PSDrawLine(fp, offx - 3, ticpixel, offx + 3, ticpixel);
		sprintf(buf, "%g", tic);
		PSDrawYAxisLabel(fp, buf, offx, ticpixel);
	}
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

void PSDrawXGrid(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height, float x0, float x1)
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
			PSDrawLine(fp, offx + x, offy2 + height - ylim/5, offx + x, offy2 + height - ylim);
		}
	}
}

void PSDrawYGrid(FILE *fp, int offx, int offy, int offx2, int offy2, int width, int height)
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
		PSDrawLine(fp, offx, offy2 + height - y, offx + width, offy2 + height - y);
	}
}

void PSInit_parent(FILE *fp)
{
	fprintf(fp, parentprolog);
}

void PSIncludeEPSF(FILE *fp, char *filename, int xpos, int ypos, int width, int height)
{
	/*
	** Set up our coordinate system
	*/
	fprintf(fp, "BeginEPSF\n");
	fprintf(fp, "%d %d translate\n", xpos, ypos);
	fprintf(fp, "0 rotate\n");
	fprintf(fp, "1 dup scale\n");
	fprintf(fp, "-18 -18 translate\n");

	/*
	** Set up a clipping rectangle
	*/
	/*
	fprintf(fp, "0 0 moveto\n");
	fprintf(fp, "%d 0 lineto\n", width);
	fprintf(fp, "%d %d lineto\n", width, height);
	fprintf(fp, "0 %d lineto\n", height);
	fprintf(fp, "closepath clip\n");
	fprintf(fp, "newpath\n");
	*/
	fprintf(fp, "%%BeginDocument: %s\n", filename);
}

void PSFinishEPSF(FILE *fp)
{
	fprintf(fp, "%%EndDocument\n");
	fprintf(fp, "EndEPSF\n");
}

void PSSet_Pref(char *property, char *value)
{
	if (!strcmp(property, "DefDotType"))
	{
		if (!strcasecmp(value, "dot"))
			pref_dottype = 0;
		if (!strcasecmp(value, "halfdash"))
			pref_dottype = 1;
		if (!strcasecmp(value, "dash"))
			pref_dottype = 2;
	}
	else if (!strcmp(property, "DashHeight"))
	{
		if (sscanf(value, "%f", &pref_dashheight) != 1)
			pref_dashheight = 1.0;
	}
}

