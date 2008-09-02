/*
**  xm_attach.c -- specify XmForm child attachments.
**
**  Copyright 1992-1994 by Jonathan Ross <cjross@bbn.com>
**
**  Permission is granted for any use not related
**  to plagiarism or litigation.
*/

#include <stdio.h>
#include <stdarg.h>

#include <X11/Intrinsic.h>
#include <Xm/Form.h>

void xm_attach (Widget w, char *where, ...)
{
    Arg		xtargs[12];	/*  collected attachment args		*/
    int		n_xtargs = 0;	/*  count thereof			*/
    int		i, j;		/*  random counters			*/
    va_list	fnargs;		/*  args to this function		*/

    /*  For each of top, left, right, bottom -- list the resources for
     *  attachment, attach-by-offset, attach-at-position, and attach-to-
     *  widget.
     */
	static struct {
		char *attach_res, *offset_res;
		char *pos_res, *widget_res;
	} res_info[] = {
		{ XmNtopAttachment, XmNtopOffset,
			  XmNtopPosition, XmNtopWidget },
		{ XmNleftAttachment, XmNleftOffset,
			  XmNleftPosition, XmNleftWidget },
		{ XmNrightAttachment, XmNrightOffset,
			  XmNrightPosition, XmNrightWidget },
		{ XmNbottomAttachment, XmNbottomOffset,
			  XmNbottomPosition, XmNbottomWidget }
	};

    /*  For each attachment type, list the XmForm attachment code, and
     *  whether or not it uses an offset, a position, and/or a widget.
     */
	static struct {
		char ch;
		int type;
		Bool uses_offset, uses_pos, uses_widget;
	} att_info[] = {
		{ 'n',	XmATTACH_NONE,			0,0,0 },
		{ 'f',	XmATTACH_FORM,			1,0,0 },
		{ 'F',	XmATTACH_OPPOSITE_FORM,		1,0,0 },
		{ 'w',	XmATTACH_WIDGET,		1,0,1 },
		{ 'W',	XmATTACH_OPPOSITE_WIDGET,	1,0,1 },
		{ 'p',	XmATTACH_POSITION,		0,1,0 },
		{ 's',	XmATTACH_SELF,			0,0,0 }
	};

    va_start(fnargs, where);

    /*  For each of the four attachment chars:
     */
    for ( i = 0; i < 4; ++i, ++where )
	{
		if (*where == '-')
			continue;

		/*  Find the corresponding entry in att_info, and add the basic
		*  attachment resource to xtargs.
		*/

		for ( j = 0; j < XtNumber(att_info); j++ )
			if (*where == att_info[j].ch)
				break;

		if (j == XtNumber(att_info))
		{
			fprintf(stderr, "xm_attach: bad %s char: '%c'\n",
				res_info[i].attach_res, *where);
			n_xtargs = 0;
			break;
		}

		XtSetArg(xtargs[n_xtargs], res_info[i].attach_res, att_info[j].type);
		++n_xtargs;

		/*  If this attachment type uses a widget, read the widget from the
		*  args list and add it to xtargs.  Do likewise if it uses an
		*  offset or position.
		*/
		if (att_info[j].uses_widget)
		{
			Widget w_val = va_arg(fnargs, Widget);
			XtSetArg(xtargs[n_xtargs], res_info[i].widget_res, w_val);
			++n_xtargs;
		}

		if (att_info[j].uses_offset)
		{
			int i_val = va_arg(fnargs, int);
			XtSetArg(xtargs[n_xtargs], res_info[i].offset_res, i_val);
			++n_xtargs;
		}
		else if (att_info[j].uses_pos)
		{
			int i_val = va_arg(fnargs, int);
			XtSetArg(xtargs[n_xtargs], res_info[i].pos_res, i_val);
			++n_xtargs;
		}
	}

    va_end(fnargs);
    if (n_xtargs > 0)
		XtSetValues(w, xtargs, n_xtargs);
}

