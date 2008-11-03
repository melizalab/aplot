
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#ifdef linux
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#endif
#include <time.h>
#include <errno.h>

#include "aplot.h"

#include <Xm/Scale.h>
#include <Xm/Form.h>
#include <Xm/ScrollBar.h>


typedef struct
{
	int ptype_mask;
	char *plot_type;
	int (*plot_open)();
} PTYPES;

PTYPES ptypes[] =
{
	{TYP_SONO, 	"sono",		sono_open},
	{TYP_OSC, 	"osc",		osc_open},
	{TYP_ISI, 	"isi",		isi_open},
	{TYP_CCOR, 	"ccor",		ccor_open},
	{TYP_PSTH, 	"psth",		psth_open},
	{TYP_RAST, 	"rast",		rast_open},
	{TYP_PSIP, 	"psip",		psip_open},
	{TYP_LABEL,	"label",	label_open},
	{TYP_VIDEO,	"video",	video_open},
	{0,			NULL,		NULL}
};


void slider_changed(PANEL *panel, int value, int slider_size)
{
	GROUP *group;
	PLOT *plot;
	float max_stop = 0, min_start = 0;
	float tmin = 0, tmax = 0;

	if (panel->groups_sel)
	{
		max_stop = panel->groups_sel->stop;
		min_start = panel->groups_sel->start;
	}
	for (group = panel->groups_sel; group != NULL; group = group->next_sel)
	{
		if (group->start < min_start) min_start = group->start;
		if (group->stop > max_stop) max_stop = group->stop;
	}
	tmin = ((value / 1000.0) * (max_stop - min_start)) + min_start;
	tmax = ((slider_size / 1000.0) * (max_stop - min_start)) + tmin;
	for (group = panel->groups_sel; group != NULL; group = group->next_sel)
	{
		if ((tmin == group->xrangemin) && (tmax == group->xrangemax))
			continue;
		group->zoomhist_count = 0;
		group->xrangemin = tmin;
		group->xrangemax = tmax;
		for (plot = group->plots; plot != NULL; plot = plot->next)
			(*(plot->plot_set))(plot);
	}
}

void new_scale_value(Widget scale_w, XtPointer client_data, XtPointer call_data)
{
	XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *)call_data;
	PANEL *pan = (PANEL *)client_data;
	Widget sb = pan->panel_scrollbar;
	int increment, maximum, minimum, page_incr, slider_size, value;

	increment = maximum = minimum = page_incr = slider_size = value = 0;
	XtVaGetValues(sb,
		XmNincrement, &increment,
		XmNmaximum, &maximum, 
		XmNminimum, &minimum,
		XmNpageIncrement, &page_incr,
		XmNsliderSize, &slider_size,
		XmNvalue, &value,
		NULL);
	slider_size = cbs->value;
	page_incr = cbs->value;
	if (value + slider_size > maximum)
		value = maximum - slider_size;
	XtVaSetValues(sb,
		XmNsliderSize, slider_size,
		XmNpageIncrement, page_incr,
		XmNvalue, value,
		NULL);
	XtVaGetValues(sb,
		XmNsliderSize, &slider_size,
		XmNvalue, &value,
		NULL);
	slider_changed(pan, value, slider_size);
}

void new_scrollbar_value(Widget scrollbar_w, XtPointer client_data, XtPointer call_data)
{
	//XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;
	PANEL *pan = (PANEL *)client_data;
	int increment, maximum, minimum, page_incr, slider_size, value;

	increment = maximum = minimum = page_incr = slider_size = value = 0;
	XtVaGetValues(scrollbar_w,
		XmNincrement, &increment,
		XmNmaximum, &maximum, 
		XmNminimum, &minimum,
		XmNpageIncrement, &page_incr,
		XmNsliderSize, &slider_size,
		XmNvalue, &value,
		NULL);
	slider_changed(pan, value, slider_size);
}

int panel_open(PANEL *pan)
{
	int status = SUCCESS;
	Arg wargs[10];
	int n;
	Display *dpy = XtDisplay(toplevel);
	Widget scale, form, scrollbar;

	n = 0;
	XtSetArg(wargs[n], XtNallowShellResize, TRUE); n++;
	XtSetArg(wargs[n], XtNtitle, pan->name); n++;
	XtSetArg(wargs[n], XmNdeleteResponse, XmDO_NOTHING); n++;
	pan->panel_shell = XtAppCreateShell("aplotwin", NULL, topLevelShellWidgetClass, dpy, wargs, n);

	if (defaults.showsliders)
	{
		form = XmCreateForm(pan->panel_shell, "panform", NULL, 0);

		n = 0;
		XtSetArg(wargs[n], XmNallowResize, TRUE); n++;
		pan->panel_container = XtCreateManagedWidget("panel", xmPanedWindowWidgetClass, form, wargs, n);

		n = 0;
		XtSetArg(wargs[n], XmNmaximum, 1000); n++;
		XtSetArg(wargs[n], XmNminimum, 1); n++;
		XtSetArg(wargs[n], XmNvalue, 1000); n++;
		XtSetArg(wargs[n], XmNdecimalPoints, 1); n++;
		XtSetArg(wargs[n], XmNshowValue, False); n++;
		XtSetArg(wargs[n], XmNorientation, XmHORIZONTAL); n++;
		XtSetArg(wargs[n], XmNprocessingDirection, XmMAX_ON_RIGHT); n++;
		XtSetArg(wargs[n], XmNtraversalOn, False); n++;
		//XtSetArg(wargs[n], XmNslidingMode, XmTHERMOMETER); n++;
		XtSetArg(wargs[n], XmNeditable, True); n++;
		scale = XtCreateManagedWidget("panscale", xmScaleWidgetClass, form, wargs, n);
		XtAddCallback(scale, XmNvalueChangedCallback, new_scale_value, pan);

		n = 0;
		XtSetArg(wargs[n], XmNorientation, XmHORIZONTAL); n++;
		XtSetArg(wargs[n], XmNminimum, 0); n++;
		XtSetArg(wargs[n], XmNmaximum, 1000); n++;
		XtSetArg(wargs[n], XmNvalue, 1000); n++;
		XtSetArg(wargs[n], XmNincrement, 10); n++;
		XtSetArg(wargs[n], XmNpageIncrement, 10); n++;
		XtSetArg(wargs[n], XmNsliderSize, 1000); n++;
		scrollbar = XmCreateScrollBar(form, "panscrollbar", wargs, n);
		XtAddCallback(scrollbar, XmNvalueChangedCallback, new_scrollbar_value, pan);
		XtAddCallback(scrollbar, XmNdragCallback, new_scrollbar_value, pan);

		XtVaSetValues(pan->panel_container,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, scale,
			NULL);

		XtVaSetValues(scale,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, scrollbar,
			NULL);

		XtVaSetValues(scrollbar,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XtManageChild(scrollbar);
		XtManageChild(form);
		pan->panel_scrollbar = scrollbar;
		pan->panel_scale = scale;
	}
	else
	{
		n = 0;
		XtSetArg(wargs[n], XmNallowResize, TRUE); n++;
		pan->panel_container = XtCreateManagedWidget("panel", xmPanedWindowWidgetClass, pan->panel_shell, wargs, n);
		pan->panel_scrollbar = NULL;
		pan->panel_scale = NULL;
	}

	pan->realized = FALSE;
	pan->groups = NULL;
	pan->nextgroupnum = 1;
	pan->numplots = 0;
	pan->numgroups = 0;
	return status;
}

int panel_close(PANEL *pan)
{
	int status = SUCCESS, tstatus;
	GROUP *group, *nextgroup;
	PLOT *plot, *nextplot;

	/*
	** For each group, close the plots, and close the sigObj
	*/
	nextgroup = NULL;
	for (group = pan->groups; group != NULL; group = nextgroup)
	{
		nextplot = NULL;
		for (plot = group->plots; plot != NULL; plot = nextplot)
		{
			if ((tstatus = (*(plot->plot_close))(plot)) != 1)
				status = tstatus;
			nextplot = plot->next;
			free(plot);
		}
		if (group->vidfp != NULL) vid_close(group->vidfp);
		if (group->pcmfp != NULL) pcm_close(group->pcmfp);
		if (group->lblfp != NULL) lbl_close(group->lblfp);
		if (group->toefp != NULL) toe_close(group->toefp);
		if (group->toe2fp != NULL) toe_close(group->toe2fp);
		if (group->zoomhist_min) free(group->zoomhist_min);
		if (group->zoomhist_max) free(group->zoomhist_max);
		nextgroup = group->next;
		free(group);
	}

	/*
	** Clear the panel
	*/
	XtUnrealizeWidget(pan->panel_shell);
	if (pan->panel_scale) XtDestroyWidget(pan->panel_scale);
	if (pan->panel_scrollbar) XtDestroyWidget(pan->panel_scrollbar);
	XtDestroyWidget(pan->panel_container);
	XtDestroyWidget(pan->panel_shell);
	pan->realized = FALSE;
	pan->groups = NULL;
	pan->nextgroupnum = 1;
	pan->numplots = 0;
	pan->numgroups = 0;

	return status;
}

int panel_addplotgroup(PANEL *pan, char *filename, long entry, char *types, GROUP **group_p, char *filename2)
{
	int status = SUCCESS, tstatus;
	GROUP *newgroup, *group;
	PLOT **newplot_p, *newplot;
	char *ch;
	int i;

	/*
	** Allocate and initialize the new group structure.
	*/
	*group_p = NULL;
	if ((newgroup = (GROUP *)calloc(sizeof(GROUP), 1)) == NULL)
		return ERROR;
	newgroup->groupnum = pan->nextgroupnum++;
	strcpy(newgroup->filename, filename);
	(newgroup->loadedfilename)[0] = '\0';
	newgroup->entry = entry;

	if (filename2 != NULL)
	{
		strcpy(newgroup->filename2, filename2);
		(newgroup->loadedfilename2)[0] = '\0';
		newgroup->entry = entry;
	}
	else
	{
		newgroup->filename2[0] = '\0';
		newgroup->loadedfilename2[0] = '\0';
		newgroup->entry2 = 0;
	}

	newgroup->plots = NULL;
	newgroup->psth_bindur = defaults.psth_bindur;
	newgroup->isi_bindur = defaults.isi_bindur;
	newgroup->fftsize = defaults.fftsize;
	newgroup->fftolap = defaults.fftolap;
	newgroup->start = defaults.xmin;
	newgroup->stop = defaults.xmax;
	newgroup->ymin = defaults.ymin;
	newgroup->ymax = defaults.ymax;
	newgroup->gsmin = defaults.gsmin;
	newgroup->gsmax = defaults.gsmax;
	newgroup->xrangemin = defaults.xrangemin;
	newgroup->xrangemax = defaults.xrangemax;
	newgroup->zoomhist_size = 20;
	newgroup->zoomhist_count = 0;
	newgroup->zoomhist_min = (float *)malloc(newgroup->zoomhist_size * sizeof(float));
	newgroup->zoomhist_max = (float *)malloc(newgroup->zoomhist_size * sizeof(float));
	newgroup->next = NULL;
	newgroup->next_sel = NULL;
	newgroup->vidfp = NULL;
	newgroup->pcmfp = NULL;
	newgroup->lblfp = NULL;
	newgroup->toefp = NULL;
	newgroup->toe2fp = NULL;
	newgroup->samples = NULL;
	newgroup->nsamples = 0;
	newgroup->ntime = 0LL;
	newgroup->lbltimes = NULL;
	newgroup->lblnames = NULL;
	newgroup->lblcount = 0;
	newgroup->toe_repptrs = NULL;
	newgroup->toe_repcounts = NULL;
	newgroup->toe_nreps = newgroup->toe_nunits = 0;
	newgroup->toe_datacount = 0;
	newgroup->toe_curunit = 1;
	newgroup->toe2_repptrs = NULL;
	newgroup->toe2_repcounts = NULL;
	newgroup->toe2_nreps = newgroup->toe2_nunits = 0;
	newgroup->toe2_datacount = 0;
	newgroup->toe2_curunit = 1;
	newgroup->needpcm = newgroup->needlbl = newgroup->needtoe = newgroup->needvid = 0;
	*group_p = newgroup;

	/*
	** Insert the new group into the panel's group list
	*/
	if ((group = pan->groups) == NULL)
		pan->groups = newgroup;
	else
	{
		while (group->next != NULL)
			group = group->next;
		group->next = newgroup;
	}
	pan->numgroups++;

	/*
	** Open the plots of this group
	*/
	newplot_p = &(newgroup->plots);
	*newplot_p = NULL;
	for (ch = types; (ch) && (*ch); ch = strchr(ch, ' ') + 1)
	{
		for (i=0; ptypes[i].plot_type; i++)
		{
			if (!strncmp(ch, ptypes[i].plot_type, strlen(ptypes[i].plot_type)))
			{
				/*
				** Allocate a plotdata structure
				*/
				if ((*newplot_p = newplot = (PLOT *)calloc(sizeof(PLOT), 1)) == NULL)
					return ERROR;
				newplot->next = NULL;
				strcpy(newplot->name, filename);
				newplot->type = ptypes[i].ptype_mask;
				strcpy(newplot->typename, ptypes[i].plot_type);
				newplot->entry = entry;
				newplot->group = newgroup;
				newplot->panel = pan;
				newplot->plot_open = ptypes[i].plot_open;
				newplot->noiseclip = FALSE;
				newplot->plot_clearmarks = NULL;
				newplot->plot_drawstartmark = NULL;
				newplot->plot_drawstopmark = NULL;
				newplot->plot_playmarker = NULL;
				newplot->plot_event_handler = NULL;
				newplot->plot_showvideoframe = NULL;
				newplot->created = FALSE;
				if ((tstatus = (*(newplot->plot_open))(newplot)) == SUCCESS)
				{
					/* If everything succeeded, move on to the next plot */
					newplot->created = TRUE;
					newgroup->numplots++;
					pan->numplots++;
					newplot_p = &(newplot->next);
					strcat(newgroup->group_plottypes, ptypes[i].plot_type);
					strcat(newgroup->group_plottypes, " ");
				}
				else
				{
					/* If there is an error, clean up and try to make the next plot */
					status = tstatus;
					free(newplot);
					*newplot_p = NULL;
				}
				break;
			}
		}
	}
	if ((pan->numplots > 0) && (XtIsRealized(pan->panel_shell) == FALSE))
	{
		XtRealizeWidget(pan->panel_shell);
		pan->realized = TRUE;
	}
	return status;
}


int panel_deleteplotgroup(PANEL *pan, long groupnum)
{
	int status = SUCCESS, tstatus;
	GROUP *group, *prevgroup, *nextgroup;
	PLOT *plot, *nextplot;

	/*
	** Find the group in the panel's grouplist
	*/
	prevgroup = group = pan->groups;
	nextgroup = NULL;
	for (prevgroup = group = pan->groups; (group != NULL); group = nextgroup)
	{
		nextgroup = group->next;
		if (group->groupnum == groupnum)
		{
			/*
			** Remove from list
			*/
			if (group == pan->groups)
				pan->groups = group->next;
			else
				prevgroup->next = group->next;

			/*
			** Close the plots
			*/
			nextplot = NULL;
			for (plot = group->plots; plot != NULL; plot = nextplot)
			{
				if ((tstatus = (*(plot->plot_close))(plot)) != 1)
					status = tstatus;
				nextplot = plot->next;
				free(plot);
				pan->numplots--;
			}

			/*
			** Close the DATAIO files, which also frees any data they allocated. (eg: the PCM samples, etc.)
			*/
			if (group->vidfp != NULL)
			{
				vid_close(group->vidfp);
				group->vidfp = NULL;
			}
			if (group->pcmfp != NULL)
			{
				pcm_close(group->pcmfp);
				group->pcmfp = NULL;
			}
			if (group->lblfp != NULL)
			{
				lbl_close(group->lblfp);
				group->lblfp = NULL;
			}
			if (group->toefp != NULL)
			{
				toe_close(group->toefp);
				group->toefp = NULL;
			}
			if (group->toe2fp != NULL)
			{
				toe_close(group->toe2fp);
				group->toe2fp = NULL;
			}

			if (group->zoomhist_min)
				free(group->zoomhist_min);
			if (group->zoomhist_max)
				free(group->zoomhist_max);
			group->zoomhist_size = 0;
			group->zoomhist_count = 0;
			free(group);
			pan->numgroups--;
			break;
		}
		prevgroup = group;
	}

	/*
	** Clear the panel, if the last plot has been removed.
	*/
	if (pan->numgroups == 0)
	{
		pan->groups = NULL;
		pan->nextgroupnum = 1;
	}

	return status;
}


int panel_display(PANEL *pan)
{
	int status = SUCCESS, tstatus;
	PLOT *plot;
	GROUP *group;

	cursor_set_busy(toplevel);
	if (pan->numgroups >= 1) cursor_set_busy(pan->panel_shell);

	for (group = pan->groups; group; group = group->next)
	{
		if (group->dirty == 1)
		{
			group->pcmdirty = group->lbldirty = group->toedirty = group->viddirty = 1;
			for (plot = group->plots; plot; plot = plot->next)
				if ((tstatus = (*(plot->plot_display))(plot)) != SUCCESS)
					status = tstatus;
			group->pcmdirty = group->lbldirty = group->toedirty = group->viddirty = 0;
			group->dirty = 0;
		}
		else
		{
			for (plot = group->plots; plot; plot = plot->next)
			{
				if (plot->dirty == 1)
					if ((tstatus = (*(plot->plot_display))(plot)) != SUCCESS)
						status = tstatus;
			}
		}
	}

	if (pan->numgroups >= 1) cursor_unset_busy(pan->panel_shell);
	cursor_unset_busy(toplevel);

	return status;
}

int panel_set(PANEL *pan, char *types, int modifiermask)
{
	int status = SUCCESS, tstatus;
	GROUP *group;
	PLOT *plot;

	cursor_set_busy(toplevel);
	if (pan->numgroups >= 1) cursor_set_busy(pan->panel_shell);

	for (group = pan->groups_sel; group; group = group->next_sel)
		for (plot = group->plots; plot; plot = plot->next)
			if ((types == NULL) || (strstr(types, plot->typename) != NULL))
				if ((tstatus = (*(plot->plot_set))(plot)) != SUCCESS)
					status = tstatus;

	if (pan->numgroups >= 1) cursor_unset_busy(pan->panel_shell);
	cursor_unset_busy(toplevel);

	return status;
}

int panel_print(PANEL *pan, int printtype, char *filename)
{
	int status = SUCCESS;
	GROUP *group;
	PLOT *plot;
	char *fname, *cmd;
	FILE *fp;
	Dimension width, height;
	int totalwidth, totalheight;

	cursor_set_busy(toplevel);
	if (pan->numgroups >= 1) cursor_set_busy(pan->panel_shell);

	fp = NULL;
	fname = NULL;
	switch (printtype)
	{
		case PRINT_TO_LPR: fname = strdup("/tmp/aplotpsXXXXXX"); mkstemp(fname); break;
		case PRINT_TO_SINGLE: fname = strdup(filename); break;
		case PRINT_TO_XFIG: fname = strdup(filename); break;
		case PRINT_TO_MANY: break;
	}
	if (fname != NULL)
	{
		if ((fp = fopen(fname, "w")) != NULL)
		{
			/*
			** Compute width, height of panel by scanning through each plot
			*/
			totalwidth = 0;
			totalheight = 0;
			for (group = pan->groups; (group != NULL) && (status == SUCCESS); group = group->next)
			{
				for (plot = group->plots; (plot != NULL) && (status == SUCCESS); plot = plot->next)
				{
					width = plot->offx + plot->offx2 + plot->width;
					height = plot->offy + plot->offy2 + plot->height;
					totalwidth = width;
					totalheight += height;
				}
				if (((printtype == PRINT_TO_SINGLE) || (printtype == PRINT_TO_LPR)) &&
					(resource_data.grouplbl == 1))
					totalheight += group->plots->minoffy2;
			}

			if (printtype == PRINT_TO_XFIG)
			{
				char *plotfname = NULL;
				int x0, y0;

				fprintf(fp, "#FIG 3.2\n");
				fprintf(fp, "Portrait\n");	/* Portrait vs. Landscape */
				fprintf(fp, "Center\n");	/* "Center" or "Flush Left" */
				fprintf(fp, "Inches\n");	/* Inches vs. Metric */
				fprintf(fp, "Letter\n");	/* Papersize */
				fprintf(fp, "100.00\n");	/* Export/Print magnification */
				fprintf(fp, "Single\n");	/* Single vs. Multiple pages */
				fprintf(fp, "-2\n");		/* No transparent color */
				fprintf(fp, "# aplot generated this\n");
				fprintf(fp, "1200 2\n");	/* Resolution / coordinate system (origin at upper left) */

				/* Now need to print the individual eps files */
				totalwidth *= (1200.0 / 72.0);
				totalheight *= (1200.0 / 72.0);
				x0 = 0;
				y0 = 0;
				fprintf(fp, "6 %d %d %d %d\n", x0, y0, x0 + totalwidth, y0 + totalheight);	/* Compound object */
				for (group = pan->groups; (group != NULL) && (status == SUCCESS); group = group->next)
				{
					for (plot = group->plots; (plot != NULL) && (status == SUCCESS); plot = plot->next)
					{
						/* Polyline object, subtype imported picture */
						fprintf(fp, "2 5 0 1 0 -1 102 0 -1 0.000 0 0 -1 0 0 5\n");
						status = (*(plot->plot_print))(plot, NULL, &plotfname);
						fprintf(fp, "\t0 %s\n", plotfname);
						free(plotfname);
						width = plot->offx + plot->offx2 + plot->width;
						height = plot->offy + plot->offy2 + plot->height;
						width *= (1200.0 / 72.0);
						height *= (1200.0 / 72.0);
						fprintf(fp, "\t%d %d %d %d %d %d %d %d %d %d\n", x0, y0, x0 + width, y0, x0 + width, y0 + height, x0, y0 + height, x0, y0);
						y0 += height;
					}
				}
				fprintf(fp, "-6\n");	/* End of compound object */
			}
			else
			{
				PSInit(fp, fname, totalwidth, totalheight);
				PSInit_parent(fp);
				for (group = pan->groups; (group != NULL) && (status == SUCCESS); group = group->next)
				{
					for (plot = group->plots; (plot != NULL) && (status == SUCCESS); plot = plot->next)
					{
						width = plot->offx + plot->offx2 + plot->width;
						height = plot->offy + plot->offy2 + plot->height;
						totalheight -= height;
						PSIncludeEPSF(fp, plot->name, 0, totalheight, width, height);
						status = (*(plot->plot_print))(plot, fp, NULL);
						PSFinishEPSF(fp);
					}
					if (resource_data.grouplbl == 1)
					{
						totalheight -= group->plots->minoffy2;
						PSDrawHorizText(fp, group->loadedfilename, 0, totalheight + 6);
					}
				}
				PSFinish(fp);
			}

			fclose(fp);
			if (printtype == PRINT_TO_LPR)
			{
				cmd = (char *)malloc(strlen(fname) + 80);
				sprintf(cmd, "lpr -r %s &", fname);
				system(cmd);
				free(cmd);
				cmd = NULL;
			}
		}
		free(fname);
		fname = NULL;
	}
	else
	{
		for (group = pan->groups; (group != NULL) && (status == SUCCESS); group = group->next)
			for (plot = group->plots; (plot != NULL) && (status == SUCCESS); plot = plot->next)
				status = (*(plot->plot_print))(plot, NULL, NULL);
	}

	if (pan->numgroups >= 1) cursor_unset_busy(pan->panel_shell);
	cursor_unset_busy(toplevel);

	return status;
}

int panel_save(PANEL *pan)
{
	int status = SUCCESS;
	GROUP *group;
	PLOT *plot;

	for (group = pan->groups; (group != NULL) && (status == SUCCESS); group = group->next)
		for (plot = group->plots; (plot != NULL) && (status == SUCCESS); plot = plot->next)
			status = (*(plot->plot_save))(plot);
	return status;
}

#define CLIPTHRESH 1000

int panel_play(PANEL *pan)
{
	int status = SUCCESS;
	GROUP *group;
	PLOT *plot, *plot1, *plot2, *plot3, *plot4;
	float playtime1, playtime2, playtime3, playtime4;
	short buf[SND_FRAMES_PER_BUFFER * 2], *samples, *ptr;
	int i, samplerate, nsamples, channels;
	int reqoffset1, reqoffset2, reqoffset3, reqoffset4;

	cursor_set_busy(toplevel);
	if (pan->numgroups >= 1) cursor_set_busy(pan->panel_shell);

	fprintf(stdout, "Start playback\n");
	/*
	** We can mix up to four channels.  Find up to four different groups
	** that are selected and ready to supply PCM data
	*/
	plot = plot1 = plot2 = plot3 = plot4 = NULL;
	playtime1 = playtime2 = playtime3 = playtime4 = 0.0;
	for (group = pan->groups_sel; group; group = group->next_sel)
	{
		if ((group->samples != NULL) && (group->nsamples > 0))
		{
			plot = group->plots;
			for (plot = group->plots; plot != NULL; plot = plot->next)
				if (plot->plot_play != NULL)
					break;
			if (plot == NULL)
				continue;
			if (plot1 == NULL)
				plot1 = plot;
			else if (plot2 == NULL)
				plot2 = plot;
			else if (plot3 == NULL)
				plot3 = plot;
			else if (plot4 == NULL)
				plot4 = plot;
			else break;
		}
	}
	status = SUCCESS;
	if (plot1 == NULL)
		goto PLAYFINISHED;

	/*
	** We can play in either mono (1 channel only) or stereo (multiple channels)
	*/
	channels = 2;
	if (plot2 == NULL)
		channels = 1;
	samplerate = plot1->group->samplerate;
	if (samplerate == 0) samplerate = 20000;

	init_sound(samplerate, channels);

	/*
	** We play in stereo if we have more than one channel.  The second and fourth channels
	** go to the right speaker; the first and third to the left.
	** We keep cycling through the four channels (plots), asking them for data to play.
	** We are finished when all of them are finished.
	** There's some trickery here to make sure things are smooth if some channels finish
	** before the others. 
	*/
	reqoffset1 = reqoffset2 = reqoffset3 = reqoffset4 = 0;
	for(;;)
	{
		memset(buf, '\0', SND_FRAMES_PER_BUFFER * 2 * sizeof(short));
		if (plot1 != NULL)
		{
			status = (*(plot1->plot_play))(plot1, reqoffset1, SND_FRAMES_PER_BUFFER,
						       &samples, &nsamples, &playtime1);
			if (nsamples == 0)
			{
				panel_playmarker(plot1->group, -1.0);
				plot1 = NULL;
			}
			else
			{
				reqoffset1 += nsamples;
				if (plot1->noiseclip == FALSE)
					for (i=0, ptr = buf; i < nsamples; i++, ptr += channels)
						*ptr = samples[i];
				else
					for (i=0, ptr = buf; i < nsamples; i++, ptr += channels)
						*ptr = ((samples[i] > CLIPTHRESH)||(samples[i] < -CLIPTHRESH)) ? samples[i] : 0;
			}
		}
		samples = NULL; nsamples = 0;
		if (plot2 != NULL)
		{
			status = (*(plot2->plot_play))(plot2, reqoffset2, SND_FRAMES_PER_BUFFER, &samples, &nsamples, &playtime2);
			if (nsamples == 0)
			{
				panel_playmarker(plot2->group, -1.0);
				plot2 = NULL;
			}
			else
			{
				reqoffset2 += nsamples;
				if (plot2->noiseclip == FALSE)
					for (i=0, ptr = buf+1; i < nsamples; i++, ptr += 2)
						*ptr = samples[i];
				else
					for (i=0, ptr = buf+1; i < nsamples; i++, ptr += 2)
						*ptr = ((samples[i] > CLIPTHRESH)||(samples[i] < -CLIPTHRESH)) ? samples[i] : 0;
			}
		}
		samples = NULL; nsamples = 0;
		if (plot3 != NULL)
		{
			status = (*(plot3->plot_play))(plot3, reqoffset3, SND_FRAMES_PER_BUFFER,
						       &samples, &nsamples, &playtime3);
			if (nsamples == 0)
			{
				panel_playmarker(plot3->group, -1.0);
				plot3 = NULL;
			}
			else
			{
				reqoffset3 += nsamples;
				if (plot3->noiseclip == FALSE)
					for (i=0, ptr = buf; i < nsamples; i++, ptr += 2)
						*ptr += samples[i];
				else
					for (i=0, ptr = buf; i < nsamples; i++, ptr += 2)
						*ptr = ((samples[i] > CLIPTHRESH)||(samples[i] < -CLIPTHRESH)) ? samples[i] : 0;
			}
		}
		samples = NULL; nsamples = 0;
		if (plot4 != NULL)
		{
			status = (*(plot4->plot_play))(plot4, reqoffset4, SND_FRAMES_PER_BUFFER,
						       &samples, &nsamples, &playtime4);
			if (nsamples == 0)
			{
				panel_playmarker(plot4->group, -1.0);
				plot4 = NULL;
			}
			else
			{
				reqoffset4 += nsamples;
				if (plot4->noiseclip == FALSE)
					for (i=0, ptr = buf+1; i < nsamples; i++, ptr += 2)
						*ptr += samples[i];
				else
					for (i=0, ptr = buf+1; i < nsamples; i++, ptr += 2)
						*ptr = ((samples[i] > CLIPTHRESH)||(samples[i] < -CLIPTHRESH)) ? samples[i] : 0;
			}
		}
		samples = NULL; nsamples = 0;
		if ((plot1 == NULL) && (plot2 == NULL) && (plot3 == NULL) && (plot4 == NULL))
			break;
		if (write_sound(buf, SND_FRAMES_PER_BUFFER) != 0)
			break;

		if (plot1 != NULL) panel_playmarker(plot1->group, playtime1);
		if (plot2 != NULL) panel_playmarker(plot2->group, playtime2);
		if (plot3 != NULL) panel_playmarker(plot3->group, playtime3);
		if (plot4 != NULL) panel_playmarker(plot4->group, playtime4);
	}
	close_sound();

PLAYFINISHED:
	if (pan->numgroups >= 1) cursor_unset_busy(pan->panel_shell);
	cursor_unset_busy(toplevel);

	return status;
}

void panel_clearmarks(PANEL *panel)
{
	GROUP *group;
	PLOT *plot;

	for (group = panel->groups_sel; group; group = group->next_sel)
		for (plot = group->plots; plot; plot = plot->next)
			if (plot->plot_clearmarks != NULL)
				(*(plot->plot_clearmarks))(plot);
}

void panel_playmarker(GROUP *group, float t)
{
	PLOT *plot;

	for (plot = group->plots; plot; plot = plot->next)
		if (plot->plot_playmarker != NULL)
			(*(plot->plot_playmarker))(plot, t);
}

void panel_showvideoframe(PANEL *panel, unsigned long long ntime)
{
	PLOT *plot;
	GROUP *group;

	for (group = panel->groups; group != NULL; group = group->next)
	{
		for (plot = group->plots; plot; plot = plot->next)
		{
			if (plot->plot_showvideoframe != NULL)
				(*(plot->plot_showvideoframe))(plot, ntime);
		}
	}
}

void panel_drawstartmark(PANEL *panel, float t)
{
	GROUP *group;
	PLOT *plot;

	for (group = panel->groups_sel; group; group = group->next_sel)
		for (plot = group->plots; plot; plot = plot->next)
			if (plot->plot_drawstartmark != NULL)
				(*(plot->plot_drawstartmark))(plot, t);
}

void panel_drawstopmark(PANEL *panel, float t)
{
	GROUP *group;
	PLOT *plot;

	for (group = panel->groups_sel; group; group = group->next_sel)
		for (plot = group->plots; plot; plot = plot->next)
			if (plot->plot_drawstopmark != NULL)
				(*(plot->plot_drawstopmark))(plot, t);
}

void panel_panleft(PANEL *panel, float percentage)
{
	GROUP *group;
	PLOT *plot;
	float tmin, tmax, tduration;
	float max_stop = 0, min_start = 0;

	if (panel->groups_sel)
	{
		max_stop = panel->groups_sel->stop;
		min_start = panel->groups_sel->start;
	}
	for (group = panel->groups_sel; group != NULL; group = group->next_sel)
	{
		if (group->start < min_start) min_start = group->start;
		if (group->stop > max_stop) max_stop = group->stop;
		tmin = group->xrangemin;
		tmax = group->xrangemax;
		tduration = tmax - tmin;
		tmin -= percentage * tduration;
		tmax -= percentage * tduration;
		if ((tmin < group->start) || (tmin > group->stop)) tmin = group->start;
		if ((tmax <= group->start) || (tmax > group->stop) || (tmax - tmin < 0.00001)) tmax = group->stop;
		group->xrangemin = tmin;
		group->xrangemax = tmax;
		for (plot = group->plots; plot != NULL; plot = plot->next)
		{
			group->xrangemin = tmin;
			group->xrangemax = tmax;
			(*(plot->plot_set))(plot);
		}
	}
	if (panel->groups_sel)
	{
		int sb_width = 1000 * (tmax - tmin) / (max_stop - min_start);
		int sb_start = 1000 * (tmin - min_start) / (max_stop - min_start);

		if (panel->panel_scale)
		{
			XtVaSetValues(panel->panel_scale,
				XmNvalue, sb_width,
				NULL);
		}
		if (panel->panel_scrollbar)
		{
			XtVaSetValues(panel->panel_scrollbar,
				XmNsliderSize, sb_width,
				XmNpageIncrement, sb_width,
				XmNvalue, sb_start,
				NULL);
		}
	}
}

void panel_panright(PANEL *panel, float percentage)
{
	GROUP *group;
	PLOT *plot;
	float tmin, tmax, tduration;
	float max_stop = 0, min_start = 0;

	if (panel->groups_sel)
	{
		max_stop = panel->groups_sel->stop;
		min_start = panel->groups_sel->start;
	}
	for (group = panel->groups_sel; group != NULL; group = group->next_sel)
	{
		if (group->start < min_start) min_start = group->start;
		if (group->stop > max_stop) max_stop = group->stop;
		tmin = group->xrangemin;
		tmax = group->xrangemax;
		tduration = tmax - tmin;
		tmin += percentage * tduration;
		tmax += percentage * tduration;
		if ((tmin < group->start) || (tmin >= group->stop)) tmin = group->start;
		if ((tmax < group->start) || (tmax > group->stop) || (tmax - tmin < 0.00001)) tmax = group->stop;
		group->xrangemin = tmin;
		group->xrangemax = tmax;
		for (plot = group->plots; plot != NULL; plot = plot->next)
		{
			group->xrangemin = tmin;
			group->xrangemax = tmax;
			(*(plot->plot_set))(plot);
		}
	}
	if (panel->groups_sel)
	{
		int sb_width = 1000 * (tmax - tmin) / (max_stop - min_start);
		int sb_start = 1000 * (tmin - min_start) / (max_stop - min_start);

		if (panel->panel_scale)
		{
			XtVaSetValues(panel->panel_scale,
				XmNvalue, sb_width,
				NULL);
		}
		if (panel->panel_scrollbar)
		{
			XtVaSetValues(panel->panel_scrollbar,
				XmNsliderSize, sb_width,
				XmNpageIncrement, sb_width,
				XmNvalue, sb_start,
				NULL);
		}
	}
}

void panel_zoom(PANEL *panel, float tmin, float tmax)
{
	GROUP *group;
	PLOT *plot;
	float max_stop = 0, min_start = 0;

	if (panel->groups_sel)
	{
		max_stop = panel->groups_sel->stop;
		min_start = panel->groups_sel->start;
	}
	for (group = panel->groups_sel; group != NULL; group = group->next_sel)
	{
		if (group->start < min_start) min_start = group->start;
		if (group->stop > max_stop) max_stop = group->stop;
		if ((tmin == group->xrangemin) && (tmax == group->xrangemax))
			continue;
		if (group->zoomhist_count < group->zoomhist_size)
		{
			group->zoomhist_min[group->zoomhist_count] = group->xrangemin;
			group->zoomhist_max[group->zoomhist_count] = group->xrangemax;
			group->zoomhist_count++;
		}
		for (plot = group->plots; plot != NULL; plot = plot->next)
		{
			group->xrangemin = tmin;
			group->xrangemax = tmax;
			(*(plot->plot_set))(plot);
		}
	}
	if (panel->groups_sel)
	{
		int sb_width = 1000 * (tmax - tmin) / (max_stop - min_start);
		int sb_start = 1000 * (tmin - min_start) / (max_stop - min_start);

		if (panel->panel_scale)
		{
			XtVaSetValues(panel->panel_scale,
				XmNvalue, sb_width,
				NULL);
		}
		if (panel->panel_scrollbar)
		{
			XtVaSetValues(panel->panel_scrollbar,
				XmNsliderSize, sb_width,
				XmNpageIncrement, sb_width,
				XmNvalue, sb_start,
				NULL);
		}
	}
}

void panel_unzoom(PANEL *panel)
{
	GROUP *group;
	PLOT *plot;
	float tmin, tmax;
	float max_stop = 0, min_start = 0;

	if (panel->groups_sel)
	{
		max_stop = panel->groups_sel->stop;
		min_start = panel->groups_sel->start;
	}
	for (group = panel->groups_sel; group != NULL; group = group->next_sel)
	{
		if (group->start < min_start) min_start = group->start;
		if (group->stop > max_stop) max_stop = group->stop;
		if (group->zoomhist_count >= 1)
		{
			group->zoomhist_count--;
			tmin = group->zoomhist_min[group->zoomhist_count];
			tmax = group->zoomhist_max[group->zoomhist_count];
		}
		else
		{
			tmin = group->start;
			tmax = group->stop;
		}
		if ((tmin == group->xrangemin) && (tmax == group->xrangemax))
			continue;
		for (plot = group->plots; plot != NULL; plot = plot->next)
		{
			group->xrangemin = tmin;
			group->xrangemax = tmax;
			(*(plot->plot_set))(plot);
		}
	}
	if (panel->groups_sel)
	{
		int sb_width = 1000 * (tmax - tmin) / (max_stop - min_start);
		int sb_start = 1000 * (tmin - min_start) / (max_stop - min_start);

		if (panel->panel_scale)
		{
			XtVaSetValues(panel->panel_scale,
				XmNvalue, sb_width,
				NULL);
		}
		if (panel->panel_scrollbar)
		{
			XtVaSetValues(panel->panel_scrollbar,
				XmNsliderSize, sb_width,
				XmNpageIncrement, sb_width,
				XmNvalue, sb_start,
				NULL);
		}
	}
}

void panel_event_handler(Widget w, XtPointer clientData, XEvent *event, Boolean *b)
{
	PANEL *panel = (PANEL *)clientData;
	GROUP *group;
	PLOT *plot;

	for (group = panel->groups; group != NULL; group = group->next)
	{
		for (plot = group->plots; plot != NULL; plot = plot->next)
		{
			if (plot->plot_widget == w)
			{
				if (plot->plot_event_handler != NULL)
				{
					(*(plot->plot_event_handler))(w, (XtPointer)plot, event, b);
				}
				return;
			}
		}
	}
	return;
}

void panel_handle_keyrelease(Widget w, PLOT *plot, XKeyEvent *keyevent, char *keybuf)
{
	PANEL *panel = plot->panel;
	GROUP *group;

	if ((keyevent->state == ShiftMask) && KEY_IS_LEFT(keybuf))
	{
		panel_panleft(panel, 0.5);
	}
	else if ((keyevent->state == ShiftMask) && KEY_IS_RIGHT(keybuf))
	{
		panel_panright(panel, 0.5);
	}
	else if (KEY_IS_LEFT(keybuf))
	{
		panel_panleft(panel, 1.0);
	}
	else if (KEY_IS_RIGHT(keybuf))
	{
		panel_panright(panel, 1.0);
	}
	else if (KEY_IS_UP(keybuf))
	{
		panel_unzoom(plot->panel);
	}
	else if (KEY_IS_DOWN(keybuf))
	{
		float tmin, tmax, ttmp;

		if ((plot->markx1 != -1) && (plot->markx2 != -1))
		{
			tmin = plot->plot_conv_pixel_to_time(plot, plot->markx1);
			tmax = plot->plot_conv_pixel_to_time(plot, plot->markx2);
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
			tmax = plot->group->xrangemin + defaults.zoomfactor * (plot->group->xrangemax - plot->group->xrangemin);
		}
		fprintf(stderr, "xrangemin = %.2f, xrangemax = %.2f, tmin = %.2f, tmax = %.2f\n", plot->group->xrangemin, plot->group->xrangemax, tmin, tmax);
		panel_zoom(plot->panel, tmin, tmax);
	}
	else if ((keyevent->state == ShiftMask) && !strcmp(keybuf, "comma"))
	{
		for (group = panel->groups_sel; group; group = group->next_sel)
		{
			if (group->entry == 1)
				return;
			group->entry -= 1;
			group->dirty = 1;
			group->zoomhist_count = 0;
			group->xrangemin = 0;
			group->xrangemax = 0;
		}
		panel_display(panel);
		update_fields(selpanel);
	}
	else if ((keyevent->state == ShiftMask) && !strcmp(keybuf, "period"))
	{
		for (group = panel->groups_sel; group; group = group->next_sel)
		{
			group->entry += 1;
			group->dirty = 1;
			group->zoomhist_count = 0;
			group->xrangemin = 0;
			group->xrangemax = 0;
		}
		panel_display(panel);
		update_fields(selpanel);
	}
	else if ((keyevent->state == ControlMask) && !strcmp(keybuf, "a"))
	{
		char title[256];

		/* If there are any marks, then clear them */
		group = plot->group;
		panel_clearmarks(panel);

		/* Now draw and set the start mark */
		panel_drawstartmark(panel, group->xrangemin);

		/* And now draw and set the stop mark */
		panel_drawstopmark(panel, group->xrangemax);

		/* Now update the title to reflect the new range */
		snprintf(title, sizeof(title), "%.2f : %.2f (%.2f)",
			group->xrangemin, group->xrangemax, group->xrangemax - group->xrangemin);
		XtVaSetValues(plot->panel->panel_shell, XmNtitle, title, NULL);
	}
	else if ((keyevent->state == ControlMask) && !strcmp(keybuf, "q"))
	{
		exit(0);
	}
	else if ((keyevent->state == ControlMask) && !strcmp(keybuf, "p"))
	{
		panel_print(panel, PRINT_TO_LPR, NULL);
	}
	else if ((keyevent->state == ShiftMask) && !strcmp(keybuf, "p"))
	{
		panel_play(panel);
	}
	else if ((keyevent->state == ShiftMask) && !strcmp(keybuf, "s"))
	{
		panel_save(panel);
	}
	else
	{
		fprintf(stderr, "panel keyevent->state = %x, keybuf = '%s'\n", keyevent->state, keybuf);
	}
}

int load_pcmdata(PCMFILE **pcmfp_p, char *loadedfilename, char *filename, int entry, 
	short **samples_p, int *nsamples_p, int *samplerate_p, float *start_p, float *stop_p,
	unsigned long long *ntime_p)
{
	PCMFILE *pcmfp;
	int rc = 0;
	int nsamples, samplerate;
	short *samples;
	unsigned long long ntime = 0LL;

	pcmfp = *pcmfp_p;
	samples = *samples_p;
	nsamples = *nsamples_p;
	samplerate = *samplerate_p;

	/*
	** If the file isn't already loaded, or it is loaded but has changed, 
	** then we need to free what we have and open the new file
	*/
	if ((samples == NULL) || (strcmp(filename, loadedfilename)))
	{
		if (samples != NULL)
			pcm_close(pcmfp);
		strcpy(loadedfilename, filename);
		if ((pcmfp = pcm_open(loadedfilename, "r")) != NULL)
		{
			pcm_ctl(pcmfp, PCMIOMMAP, NULL);
		}
		else
		{
			if ((strchr(filename, '%') != NULL) &&
				(sprintf(loadedfilename, filename, entry) != 0) &&
				((pcmfp = pcm_open(loadedfilename, "r")) != NULL))
			{
				pcm_ctl(pcmfp, PCMIOMMAP, NULL);
				entry = 1;
			}
			else
			{
				pcmfp = NULL;
				samples = NULL;
				nsamples = 0;
				rc = -1;
				fprintf(stderr, "Error opening file '%s'\n", loadedfilename);
			}
		}
	}

	/*
	** Okay, now we know the correct file is open.  We need to go to the
	** desired entry, and read the data.
	*/
	if (pcmfp != NULL)
	{
		rc = -1;
		samples = NULL;
		nsamples = 0;
		if (pcm_seek(pcmfp, entry) != -1)
		{
			errno = 0;
			if ((pcm_read(pcmfp, &samples, &nsamples) == -1) && (errno == ENOMEM))
			{
#ifdef linux
				pcm_close(pcmfp);
				if ((pcmfp = pcm_open(filename, "r")) != NULL)
					if (pcm_ctl(pcmfp, PCMIOMALLOC, NULL) != -1)
						if (pcm_seek(pcmfp, entry) != -1)
							pcm_read(pcmfp, &samples, &nsamples);
#endif
			}

			if (samples != NULL)
			{
				long timestamp;
				long microtimestamp;
				char tbuf[40], tbuf2[40];

				pcm_ctl(pcmfp, PCMIOGETSR, &samplerate);
				rc = 0;
				pcm_ctl(pcmfp, PCMIOGETTIME, &timestamp);
				microtimestamp = 0;
				pcm_ctl(pcmfp, PCMIOGETTIMEFRACTION, &microtimestamp);
				strftime(tbuf, sizeof(tbuf), "%a %b %d %Y %k:%M:%S", localtime(&timestamp));
				sprintf(tbuf2, "%02.2f", microtimestamp / 1000000.0);
				printf("%s: TIMESTAMP=%s.%s\n", loadedfilename, tbuf, tbuf2+2);
				fflush(stdout);
				ntime = (microtimestamp * 1000LL) + (timestamp * 1000000000LL);
			}
		}
	}

	*pcmfp_p = pcmfp;
	*samples_p = samples;
	*nsamples_p = nsamples;
	*samplerate_p = samplerate;
	*start_p = 0.0;
	*stop_p = (rc == 0) ? (*start_p + (nsamples * (1000.0 / samplerate))) : (0.0);
	if (ntime_p) *ntime_p = ntime;
	return rc;
}


int load_lbldata(LBLFILE **lblfp_p, char *loadedfilename, char *filename, int entry, 
	int *lblcount_p, float **lbltimes_p, char ***lblnames_p, float *start_p, float *stop_p)
{
	PCMFILE *pcmfp;
	LBLFILE *lblfp;
	int i, rc = 0, lblcount, nsamples, samplerate;
	float start, stop;
	float *lbltimes;
	char **lblnames;
	char filenamebuf[256], *ch;
	int caps;

	lblfp = *lblfp_p;
	start = *start_p;
	stop = *stop_p;
	lbltimes = *lbltimes_p;
	if (lbltimes != NULL)
		lbl_close(lblfp);
	lblcount = 0;
	lbltimes = NULL;
	lblnames = NULL;
	rc = -1;
	strcpy(loadedfilename, filename);
	if (((lblfp = lbl_open(loadedfilename, "r")) == NULL) &&
		((strchr(filename, '%') == NULL) ||
		(sprintf(loadedfilename, filename, entry) <= 0) ||
		((lblfp = lbl_open(loadedfilename, "r")) == NULL)))
	{
		/*
		** If we can't open the label file, maybe it is actually a pcm file.
		** In this case, get length info from the PCM file, but open a new/old LBL
		** file with a name derived from that of the PCM file and entry...
		*/
		if ((pcmfp = pcm_open(filename, "r")) == NULL)
			if ((pcmfp = pcm_open(loadedfilename, "r")) != NULL)
				entry = 1;
		if (pcmfp != NULL)
		{
			if (pcm_seek(pcmfp, entry) != -1)
			{
				samplerate = 20000;
				pcm_ctl(pcmfp, PCMIOGETSIZE, &nsamples);
				pcm_ctl(pcmfp, PCMIOGETSR, &samplerate);
				start = 0.0;
				stop = start + (nsamples * (1000.0 / samplerate));
				strcpy(filenamebuf, loadedfilename);
				ch = strrchr(filenamebuf, '.');
				if ((pcm_ctl(pcmfp, PCMIOGETCAPS, &caps) != -1) && ((caps & PCMIOCAP_MULTENTRY) == 0))
					sprintf(ch, ".lbl");
				else
					sprintf(ch, "_%03d.lbl", entry);
				printf("%s %d\n", filenamebuf, entry);
				if ((lblfp = lbl_open(filenamebuf, "r")) != NULL)
					lbl_read(lblfp, &lbltimes, &lblnames, &lblcount);
				strcpy(loadedfilename, filenamebuf);
				rc = 0;
			}
			pcm_close(pcmfp);
		}
	}
	else
	{
		/*
		** We were able to open the label file.  We choose as our length, the
		** longest label...  I'm not sure why I'm not assuming that the labels 
		** are sorted here (I could just use the last label as the max.
		*/
		if (lbl_read(lblfp, &lbltimes, &lblnames, &lblcount) != -1)
			rc = 0;
		if (lbltimes != NULL)
		{
			float max = 0.0;
			for (i=0; i < lblcount; i++)
				if (lbltimes[i] > max)
					max = lbltimes[i];
			if ((stop == start) || (stop < max + 1.0))
			{
				start = 0.0;
				stop = max + 1.0;
			}
		}
	}

	/*
	** Return the results
	*/
	*lbltimes_p = lbltimes;
	*lblnames_p = lblnames;
	*lblcount_p = lblcount;
	*start_p = start;
	if (stop > *stop_p) *stop_p = stop;
	*lblfp_p = lblfp;
	return rc;
}

int load_toedata(TOEFILE **toefp_p, char *loadedfilename, char *filename, int entry, int unit, 
	int *datacount_p, int *nunits_p, int *nreps_p, int **repcounts_p, float ***repptrs_p,
	float *start_p, float *stop_p)
{
	TOEFILE *toefp;
	int i, rc = 0, count;
	float start, stop, *data;
	int datacount, nunits, nreps, *repcounts;
	float **repptrs;

	/*
	** Free anything that's already loaded.  Clear variables.
	** Default to error condition.
	*/
	toefp = *toefp_p;
	repptrs = *repptrs_p;
	start = 0.0;
	stop = 0.0;

	if (repptrs != NULL)
		toe_close(toefp);
	repptrs = NULL;
	repcounts = NULL;
	nunits = 0;
	nreps = 0;
	rc = -1;

	/*
	** Read the toe file
	*/
	strcpy(loadedfilename, filename);
	if (((toefp = toe_open(loadedfilename, "r")) != NULL) ||
		((strchr(filename, '%') != NULL) &&
		(sprintf(loadedfilename, filename, entry) > 0) &&
		((toefp = toe_open(loadedfilename, "r")) != NULL)))
	{
		if (toe_seek(toefp, unit) != -1)
			if (toe_read(toefp, &repptrs, &repcounts, &nunits, &nreps) != -1)
				rc = 0;
	}

	/*
	** Count the total number of events
	*/
	for (datacount=0, i=1; i <= nreps; i++)
		datacount += repcounts[i];
	if ((stop == start) && (repptrs != NULL))
	{
		for (i=0; i < nreps; i++)
		{
			data = repptrs[i+1];
			count = repcounts[i+1];
			if (data[0] < start)
				start = data[0];
			if (data[count-1] > stop)
				stop = data[count-1];
		}
	}

	/*
	** Return the results
	*/
	*toefp_p = toefp;
	*datacount_p = datacount;
	*nunits_p = nunits;
	*nreps_p = nreps;
	*repcounts_p = repcounts;
	*repptrs_p = repptrs;
	*start_p = start;
	*stop_p = stop;
	return rc;
}


int load_viddata(VIDFILE **vidfp_p, char *loadedfilename, char *filename, int entry, 
	int *nframes_p, int *width_p, int *height_p, int *ncomps_p, int *microsecs_per_frame_p,
	unsigned long long *ntime_p)
{
	VIDFILE *vidfp;
	int nframes, width, height, ncomps, microsecs_per_frame;
	unsigned long long ntime = 0LL;

	vidfp = *vidfp_p;

	/*
	** If the file isn't already loaded, or it is loaded but has changed, 
	** then we need to free what we have and open the new file
	*/
	if ((vidfp == NULL) || (strcmp(filename, loadedfilename)))
	{
		if (vidfp != NULL)
			vid_close(vidfp);
		strcpy(loadedfilename, filename);
		if ((vidfp = vid_open(loadedfilename, "r")) == NULL)
		{
			if ((strchr(filename, '%') == NULL) ||
				(sprintf(loadedfilename, filename, entry) == 0) ||
				((vidfp = vid_open(loadedfilename, "r")) == NULL))
			{
				*vidfp_p = NULL;
				*nframes_p = 0;
				*width_p = 0;
				*height_p = 0;
				*ncomps_p = 0;
				*microsecs_per_frame_p = 0;
				fprintf(stderr, "Error opening file '%s'\n", loadedfilename);
				return -1;
			}
			entry = 1;
		}
	}

	/*
	** Okay, now we know the correct file is open.  We need to go to the
	** desired entry, and read the data.
	*/
	if (vidfp != NULL)
	{
		vid_seek(vidfp, entry);
		errno = 0;
		vid_ctl(vidfp, VIDIOGETNFRAMES, &nframes);
		vid_ctl(vidfp, VIDIOGETMICROSECPERFRAME, &microsecs_per_frame);
		vid_ctl(vidfp, VIDIOGETWIDTH, &width);
		vid_ctl(vidfp, VIDIOGETHEIGHT, &height);
		vid_ctl(vidfp, VIDIOGETNCOMPONENTS, &ncomps);
		vid_ctl(vidfp, VIDIOGETSTARTNTIME, &ntime);
	}

	*vidfp_p = vidfp;
	*nframes_p = nframes;
	*width_p = width;
	*height_p = height;
	*ncomps_p = ncomps;
	*microsecs_per_frame_p = microsecs_per_frame;
	if (ntime_p) *ntime_p = ntime;
	return 0;
}

void report_size(PLOT *pdata, char *prefix)
{
#ifdef notdef
	Dimension windowheight, windowwidth;
	Dimension plotheight, plotwidth, newheight;
	PANEL *pan = pdata->panel;
	GROUP *group = NULL;
	PLOT *plot = NULL;

	plotheight = plotwidth = windowheight = windowwidth = 0;
	XtVaGetValues(pdata->plot_widget,
		XmNheight, &plotheight,
		XmNwidth, &plotwidth,
		NULL);
	XtVaGetValues(pan->panel_container,
		XmNheight, &windowheight,
		XmNwidth, &windowwidth,
		NULL);
	fprintf(stderr, "%s plot: %3d x %3d   window: %3d x %3d\n", prefix, plotwidth, plotheight, windowwidth, windowheight);

	if (strstr(prefix, "EXPOSE"))
	{
//		XtVaSetValues(pan->panel_container,
//			XmNrefigureMode, True,
//			NULL);
		return;
	}

	XtVaSetValues(pan->panel_container,
		XmNrefigureMode, False,
		NULL);
	if ((windowheight != pan->container_height) || (windowwidth != pan->container_width))
	{
		if (windowheight != pan->container_height)
		{
			for (group = pan->groups; group != NULL; group = group->next)
			{
				for (plot = group->plots; plot; plot = plot->next)
				{
					XtVaSetValues(plot->plot_widget,
						XmNallowResize, True,
						NULL);
					XtVaSetValues(plot->plot_widget,
						XmNskipAdjust, True,
						NULL);
				}
			}
			for (group = pan->groups; group != NULL; group = group->next)
			{
				for (plot = group->plots; plot; plot = plot->next)
				{
					plotheight = newheight = 0;
					XtVaGetValues(plot->plot_widget,
						XmNheight, &plotheight,
						NULL);
					newheight = ((float)plotheight / (float)pan->container_height) * (float)windowheight;
					XtVaSetValues(plot->plot_widget,
						XmNheight, newheight,
						NULL);
					fprintf(stderr, "changing height from %d to %d\n", plotheight, newheight);
				}
			}
			for (group = pan->groups; group != NULL; group = group->next)
			{
				for (plot = group->plots; plot; plot = plot->next)
				{
//					XtVaSetValues(plot->plot_widget,
//						XmNallowResize, False,
//						NULL);
//					XtVaSetValues(plot->plot_widget,
//						XmNskipAdjust, False,
//						NULL);
				}
			}
		}
		pan->container_height = windowheight;
		pan->container_width = windowwidth;
	}
#endif
}

