
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
#include "aplot.h"
#include <Xm/XmP.h>
#include <X11/Xatom.h>
#include <time.h>

void colormap_gray(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals);
void colormap_hot(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals);
void colormap_cool(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals);
void colormap_bone(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals);

cmap_fns colormap[] = {{colormap_gray}, {colormap_hot}, {colormap_cool}, {colormap_bone}};


void colormap_gray(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals)
{
	int i, grayval, step;
	XColor color;

	for (i=0,grayval=65535,step=65535/(ncolors-1); i < ncolors; i++,grayval-=step)
	{
		color.red = color.green = color.blue = grayval;
		if ((dpy != NULL) && (colors != NULL))
		{
			XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &color);
			colors[i] = (unsigned long)color.pixel;
		}
		if (cvals != NULL)
		{
			cvals[i] =  ((color.red * (ncolors - 1) / 65535) << 16) | ((color.green * (ncolors - 1) / 65535) << 8) | (color.blue * (ncolors -1 ) / 65535);
		}
	}
}

void colormap_hot(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals)
{
	int i, n, m;
	XColor color;

	if ((dpy != NULL) && (colors != NULL))
	{
		for (i=0; i < ncolors; i++)
			colors[i] = 0;
	}

	m = ncolors;
	n = floor((3.0 / 8.0) * m);
	for (i=0; i < n; i++)
	{
		color.red = floor(((i+1.0) / (1.0 * n)) * 65535);
		color.green = 0;
		color.blue = 0;
		if ((dpy != NULL) && (colors != NULL))
		{
			XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &color);
			colors[i] = (unsigned long)color.pixel;
		}
		if (cvals != NULL)
		{
			cvals[i] =  ((color.red * (ncolors - 1) / 65535) << 16) | ((color.green * (ncolors - 1) / 65535) << 8) | (color.blue * (ncolors -1 ) / 65535);
		}
	}
	for (; i < (2*n); i++)
	{
		color.red = 65535;
		color.green = floor(((i - n + 1.0) / (1.0 * n)) * 65535);
		color.blue = 0;
		if ((dpy != NULL) && (colors != NULL))
		{
			XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &color);
			colors[i] = (unsigned long)color.pixel;
		}
		if (cvals != NULL)
		{
			cvals[i] =  ((color.red * (ncolors - 1) / 65535) << 16) | ((color.green * (ncolors - 1) / 65535) << 8) | (color.blue * (ncolors -1 ) / 65535);
		}
	}
	for (; i < m; i++)
	{
		color.red = 65535;
		color.green = 65535;
		color.blue = floor(((i - (2*n) + 1.0) / (1.0 * (m - 2*n))) * 65535);
		if ((dpy != NULL) && (colors != NULL))
		{
			XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &color);
			colors[i] = (unsigned long)color.pixel;
		}
		if (cvals != NULL)
		{
			cvals[i] =  ((color.red * (ncolors - 1) / 65535) << 16) | ((color.green * (ncolors - 1) / 65535) << 8) | (color.blue * (ncolors -1 ) / 65535);
		}
	}
}

void colormap_cool(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals)
{
	int i, m;
	XColor color;

	m = ncolors;
	for (i=0; i < m; i++)
	{
		color.red = floor((i / (1.0 * (m - 1))) * 65535);
		color.green = 65535 - color.red;
		color.blue = 65535;
		if ((dpy != NULL) && (colors != NULL))
		{
			XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &color);
			colors[i] = (unsigned long)color.pixel;
		}
		if (cvals != NULL)
		{
			cvals[i] =  ((color.red * (ncolors - 1) / 65535) << 16) | ((color.green * (ncolors - 1) / 65535) << 8) | (color.blue * (ncolors -1 ) / 65535);
		}
	}
}

void colormap_bone(Display *dpy, int ncolors, unsigned long *colors, unsigned long *cvals)
{
	int i, grayval, step;
	XColor color;

	for (i=0,grayval=65535,step=65535/(ncolors-1); i < ncolors; i++,grayval-=step)
	{
		color.red = MIN(65535, floor(1.25 * grayval));
		color.green = floor(0.7812 * grayval);
		color.blue = floor(0.4975 * grayval);
		if ((dpy != NULL) && (colors != NULL))
		{
			XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &color);
			colors[i] = (unsigned long)color.pixel;
		}
		if (cvals != NULL)
		{
			cvals[i] =  ((color.red * (ncolors - 1) / 65535) << 16) | ((color.green * (ncolors - 1) / 65535) << 8) | (color.blue * (ncolors -1 ) / 65535);
		}
	}
}

