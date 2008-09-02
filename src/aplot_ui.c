
/**********************************************************************************
 * INCLUDES
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <Xm/Xm.h> 
#include <Xm/ArrowBG.h> 
#include <Xm/DrawingA.h> 
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Scale.h>
#include <Xm/BulletinB.h>
#include <Xm/Label.h>
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/Separator.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include "aplot.h"

#ifdef VMS
#include <sys$library/DECw$Cursor.h>
#else
#include <X11/cursorfont.h>
#endif


enum
{
	WN_SEP1,
#define WDG_SEP1 children[WN_SEP1]
	WN_FILE_LBL,
#define WDG_FILE_LBL children[WN_FILE_LBL]
	WN_FILE_ADD_BUT,
#define WDG_FILE_ADD_BUT children[WN_FILE_ADD_BUT]
	WN_FILE_DROP_BUT,
#define WDG_FILE_DROP_BUT children[WN_FILE_DROP_BUT]
	WN_FILE_REWIND_BUT,
#define WDG_FILE_REWIND_BUT children[WN_FILE_REWIND_BUT]
	WN_FILE_PREV_BUT,
#define WDG_FILE_PREV_BUT children[WN_FILE_PREV_BUT]
	WN_FILE_NEXT_BUT,
#define WDG_FILE_NEXT_BUT children[WN_FILE_NEXT_BUT]
	WN_FILE_ENTRY_TEXT,
#define WDG_FILE_ENTRY_TEXT children[WN_FILE_ENTRY_TEXT]
	WN_PLOT_LBL,
#define WDG_PLOT_LBL children[WN_PLOT_LBL]
	WN_PLOT_ADD_BUT,
#define WDG_PLOT_ADD_BUT children[WN_PLOT_ADD_BUT]
	WN_PLOT_DROP_BUT,
#define WDG_PLOT_DROP_BUT children[WN_PLOT_DROP_BUT]
	WN_PLOT_SONO_TOG,
#define WDG_PLOT_SONO_TOG children[WN_PLOT_SONO_TOG]
	WN_PLOT_OSC_TOG,
#define WDG_PLOT_OSC_TOG children[WN_PLOT_OSC_TOG]
	WN_PLOT_PSTH_TOG,
#define WDG_PLOT_PSTH_TOG children[WN_PLOT_PSTH_TOG]
	WN_PLOT_RAST_TOG,
#define WDG_PLOT_RAST_TOG children[WN_PLOT_RAST_TOG]
	WN_PLOT_ISI_TOG,
#define WDG_PLOT_ISI_TOG children[WN_PLOT_ISI_TOG]
	WN_PLOT_CCOR_TOG,
#define WDG_PLOT_CCOR_TOG children[WN_PLOT_CCOR_TOG]
	WN_PLOT_PSIP_TOG,
#define WDG_PLOT_PSIP_TOG children[WN_PLOT_PSIP_TOG]
	WN_PLOT_VIDEO_TOG,
#define WDG_PLOT_VIDEO_TOG children[WN_PLOT_VIDEO_TOG]
	WN_PLOT_LABEL_TOG,
#define WDG_PLOT_LABEL_TOG children[WN_PLOT_LABEL_TOG]
	WN_PANEL_LBL,
#define WDG_PANEL_LBL children[WN_PANEL_LBL]
	WN_PANEL_NEW_BUT,
#define WDG_PANEL_NEW_BUT children[WN_PANEL_NEW_BUT]
	WN_PANEL_CLOSE_BUT,
#define WDG_PANEL_CLOSE_BUT children[WN_PANEL_CLOSE_BUT]
	WN_PANEL_PRINT_BUT,
#define WDG_PANEL_PRINT_BUT children[WN_PANEL_PRINT_BUT]
	WN_PANEL_SAVE_BUT,
#define WDG_PANEL_SAVE_BUT children[WN_PANEL_SAVE_BUT]
	WN_PANEL_PLAY_BUT,
#define WDG_PANEL_PLAY_BUT children[WN_PANEL_PLAY_BUT]

	WN_PROP_X_LBL,
#define WDG_PROP_X_LBL children[WN_PROP_X_LBL]
	WN_PROP_XMIN_TEXT,
#define WDG_PROP_XMIN_TEXT children[WN_PROP_XMIN_TEXT]
	WN_PROP_XMAX_TEXT,
#define WDG_PROP_XMAX_TEXT children[WN_PROP_XMAX_TEXT]

	WN_PROP_Y_LBL,
#define WDG_PROP_Y_LBL children[WN_PROP_Y_LBL]
	WN_PROP_YMIN_TEXT,
#define WDG_PROP_YMIN_TEXT children[WN_PROP_YMIN_TEXT]
	WN_PROP_YMAX_TEXT,
#define WDG_PROP_YMAX_TEXT children[WN_PROP_YMAX_TEXT]

	WN_PROP_FR_LBL,
#define WDG_PROP_FR_LBL children[WN_PROP_FR_LBL]
	WN_PROP_FRMIN_TEXT,
#define WDG_PROP_FRMIN_TEXT children[WN_PROP_FRMIN_TEXT]
	WN_PROP_FRMAX_TEXT,
#define WDG_PROP_FRMAX_TEXT children[WN_PROP_FRMAX_TEXT]

	WN_PROP_GS_LBL,
#define WDG_PROP_GS_LBL children[WN_PROP_GS_LBL]
	WN_PROP_GSMIN_TEXT,
#define WDG_PROP_GSMIN_TEXT children[WN_PROP_GSMIN_TEXT]
	WN_PROP_GSMAX_TEXT,
#define WDG_PROP_GSMAX_TEXT children[WN_PROP_GSMAX_TEXT]

	WN_PROP_XRANGE_LBL,
#define WDG_PROP_XRANGE_LBL children[WN_PROP_XRANGE_LBL]
	WN_PROP_XRANGEMIN_TEXT,
#define WDG_PROP_XRANGEMIN_TEXT children[WN_PROP_XRANGEMIN_TEXT]
	WN_PROP_XRANGEMAX_TEXT,
#define WDG_PROP_XRANGEMAX_TEXT children[WN_PROP_XRANGEMAX_TEXT]

	WN_QUIT_BUT,
#define WDG_QUIT_BUT children[WN_QUIT_BUT]
	WN_NUMCHILDREN
};


/*
 * Global variables
 */
Widget toplevel;
Widget parentform;
Widget children[WN_NUMCHILDREN];
Widget WDG_FILE_LIST, WDG_PANEL_LIST;
Widget WDG_FILE_DIALOG;
PANEL *panels = NULL, *selpanel = NULL;
Atom WM_DELETE_WINDOW;

ApplicationData resource_data;

XtAppContext app_con;
static char *fallback_resources[] =
{
	"Aplot*fontList: fixed\n",
#ifdef __osf__
"Aplot*background: gray76\n",
"aplotwin*background: gray76\n",
"aplotwin*fontList: fixed\n",
#endif
	NULL
};

XrmOptionDescRec im_options[] =
{
	{"-grouplbl","grouplbl",XrmoptionSepArg,NULL},
	{"-dottype","dottype",XrmoptionSepArg,NULL},
	{"-sono_geometry","sono_geometry",XrmoptionSepArg,NULL},
	{"-osc_geometry","osc_geometry",XrmoptionSepArg,NULL},
	{"-seq2","outputseq2",XrmoptionSepArg,NULL},
	{"-def","defaults_section",XrmoptionSepArg,NULL},
};

static XtResource resources[] =
{
	{XtNbackground,XtCBackground,XtRPixel,sizeof(Pixel), XtOffset(ApplicationDataPtr,bg), XtRString, "white"},
	{XtNgeometry,XtCGeometry,XtRString,sizeof(String *), XtOffset(ApplicationDataPtr,geometry),XtRString, "500x100+0+0"},
	{"sono_geometry","Sono_geometry", XtRString,sizeof(String *), XtOffset(ApplicationDataPtr,sono_geometry), XtRString, "500x100+100+0"},
	{"osc_geometry","Osc_geometry", XtRString,sizeof(String *), XtOffset(ApplicationDataPtr,osc_geometry), XtRString, "500x100+100+214"},
	{"grouplbl","Grouplbl", XtRInt,sizeof(int), XtOffset(ApplicationDataPtr,grouplbl), XtRImmediate, (caddr_t)DEFAULT_DOGROUPLABEL},
	{"dottype","Dottype", XtRString,sizeof(String *), XtOffset(ApplicationDataPtr,dottype), XtRString, DEFAULT_PSDOTTYPE},
 	{"outputseq2","Outputseq2", XtRInt, sizeof(int), XtOffset(ApplicationDataPtr,outputseq2), XtRImmediate, (caddr_t)DEFAULT_OUTPUTSEQ2},
 	{"defaults_section","Defaults", XtRInt, sizeof(int), XtOffset(ApplicationDataPtr,defaults_section), XtRImmediate, (caddr_t)DEFAULT_DEFAULTSSECTION},
};


/*
 * Prototypes
 */
static void update_selection_tree(void);

static void cb_file_dialog_ok(Widget w, void *data, XmFileSelectionBoxCallbackStruct *call_data);
static void cb_file_dialog_cancel(Widget w, void *data, XtPointer call_data);
static void cb_file_add_but(Widget w, void *data, XtPointer call_data);
static void cb_file_drop_but(Widget w, void *data, XtPointer call_data);
static void cb_file_entry_changed(Widget w, int data, XtPointer call_data);
static void cb_file_list(Widget w, XtPointer clientData, XtPointer callData);

static void cb_plot_add_but(Widget w, void *data, XtPointer call_data);
static void cb_plot_drop_but(Widget w, void *data, XtPointer call_data);

static void cb_panel_new_but(Widget w, void *data, XtPointer call_data);
static void cb_panel_close_but(Widget w, void *data, XtPointer call_data);
static void cb_panel_print_but(Widget w, void *data, XtPointer call_data);
static void cb_panel_save_but(Widget w, void *data, XtPointer call_data);
static void cb_panel_play_but(Widget w, void *data, XtPointer call_data);
static void cb_panel_list(Widget w, XtPointer clientData, XtPointer callData);
static void cb_panel_psfile_dialog_ok(Widget w, void *data, XmFileSelectionBoxCallbackStruct *call_data);
static void cb_panel_psfile_dialog_cancel(Widget w, void *data, XtPointer call_data);

static void cb_prop_zoom_but(Widget w, void *data, XtPointer call_data);

static void cb_quit_but(Widget w, void *data, XtPointer call_data);
static void cb_range_text(Widget w, void *data, XtPointer call_data);

void cursor_set_busy(Widget w);
void cursor_unset_busy(Widget w);

extern void xm_attach();


/***********************************************************************************************************/
/********************                                                            ***************************/
/********************                       I N I T _ U I                        ***************************/
/********************                                                            ***************************/
/***********************************************************************************************************/
void init_ui(int argc, char *argv[])
{
	int n, i;
	Arg args[10];
	XmString xmstr;
	int defsect;

	toplevel = XtAppInitialize(&app_con, "Aplot", im_options, XtNumber(im_options),
		&argc, argv, fallback_resources, NULL, 0);
	XtGetApplicationResources(toplevel, &resource_data, resources, XtNumber(resources), NULL, 0);
	WM_DELETE_WINDOW = XmInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", False);

	init_defaults();
	defsect = resource_data.defaults_section;
	load_defaults(defsect);

	/*
	** Process '-set' arguments to supersede defaults
	*/
	for (i=0; i < argc - 1; i++)
	{
		if (!strcmp(argv[i], "-set"))
		{
			char *ch;

			i++;
			ch = strchr(argv[i], '=');
			if (ch == NULL)
			{
				fprintf(stderr, "ERROR: incorrect syntax, expecting '=' in '%s'\n", argv[i]);
				exit(-1);
			}
			*ch = '\0';
			ch++;
			if (set_default(argv[i], ch) != 0)
			{
				fprintf(stderr, "ERROR interpreting '%s=%s'\n", argv[i], ch);
				exit(-1);
			}
		}
	}

	PSSet_Pref("DefDotType", (char *)resource_data.dottype);

	n = 0;
	XtSetArg(args[n], XmNheight, 285); n++;
	XtSetArg(args[n], XmNwidth, 525); n++;
	parentform = XmCreateForm(toplevel, "parentform", args, n);

	XtSetArg(args[0], XmNorientation, XmHORIZONTAL);
	XtSetArg(args[1], XmNseparatorType, XmSHADOW_ETCHED_IN);
	WDG_SEP1 = XmCreateSeparator(parentform, "sep1", args, 2);

	/*
	** THE FILE SECTION
	*/
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("File", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_FILE_LBL = XmCreateLabel(parentform, "file_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Add", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_FILE_ADD_BUT = XmCreatePushButton(parentform, "file_add_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_FILE_ADD_BUT, XmNactivateCallback, (XtCallbackProc) cb_file_add_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Drop", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_FILE_DROP_BUT = XmCreatePushButton(parentform, "file_drop_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_FILE_DROP_BUT, XmNactivateCallback, (XtCallbackProc) cb_file_drop_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Rewind", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_FILE_REWIND_BUT = XmCreatePushButton(parentform, "file_rewind_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_FILE_REWIND_BUT, XmNactivateCallback, (XtCallbackProc)cb_file_entry_changed, (void *)WN_FILE_REWIND_BUT);

	n = 0;
	XtSetArg(args[n], XmNarrowDirection, XmARROW_LEFT); n++;
	WDG_FILE_PREV_BUT = XmCreateArrowButtonGadget(parentform, "file_prev_but", args, n);
	XtAddCallback(WDG_FILE_PREV_BUT, XmNactivateCallback, (XtCallbackProc) cb_file_entry_changed, (void *)WN_FILE_PREV_BUT);

	n = 0;
	XtSetArg(args[n], XmNarrowDirection, XmARROW_RIGHT); n++;
	WDG_FILE_NEXT_BUT = XmCreateArrowButtonGadget(parentform, "file_next_but", args, n);
	XtAddCallback(WDG_FILE_NEXT_BUT, XmNactivateCallback, (XtCallbackProc) cb_file_entry_changed, (void *)WN_FILE_NEXT_BUT);

	n = 0;
	XtSetArg(args[n], XmNcolumns, 3); n++;
	WDG_FILE_ENTRY_TEXT = XmCreateTextField(parentform, "file_entry_text", args, n);
	XtAddCallback(WDG_FILE_ENTRY_TEXT, XmNactivateCallback, (XtCallbackProc)cb_file_entry_changed, (void *)WN_FILE_ENTRY_TEXT);

	n = 0;
	XtSetArg(args[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
	XtSetArg(args[n], XmNlistSizePolicy, XmDYNAMIC); n++;
	WDG_FILE_LIST = XmCreateScrolledList(parentform, "file_list", args, n);
	XtAddCallback(WDG_FILE_LIST, XmNextendedSelectionCallback, (XtCallbackProc) cb_file_list, NULL);

	/* Add the command line arguments to the list */
	for (i=1; i < argc; i++)
	{
		if (!strncmp(argv[i], "-set", 4))
		{
			i++;
			continue;
		}
		xmstr = XmStringGenerate(argv[i], XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL);
		XmListAddItem(WDG_FILE_LIST, xmstr, 0);
		XmStringFree(xmstr);
	}
	XmListDeselectAllItems(WDG_FILE_LIST);

	WDG_FILE_DIALOG = XmCreateFileSelectionDialog(toplevel, "file_dialog", NULL, 0);
	XtUnmanageChild(XmSelectionBoxGetChild(WDG_FILE_DIALOG, XmDIALOG_HELP_BUTTON));
	XtSetSensitive(WDG_FILE_DIALOG, True);
	XtAddCallback(WDG_FILE_DIALOG, XmNokCallback, (XtCallbackProc) cb_file_dialog_ok, NULL);
	XtAddCallback(WDG_FILE_DIALOG, XmNcancelCallback, (XtCallbackProc) cb_file_dialog_cancel, NULL);

	/*
	** THE PLOT SECTION
	*/
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Plot", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_LBL = XmCreateLabel(parentform, "plot_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Add", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_ADD_BUT = XmCreatePushButton(parentform, "plot_add_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_PLOT_ADD_BUT, XmNactivateCallback, (XtCallbackProc) cb_plot_add_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Drop", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_DROP_BUT = XmCreatePushButton(parentform, "plot_drop_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_PLOT_DROP_BUT, XmNactivateCallback, (XtCallbackProc) cb_plot_drop_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Sono", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_SONO_TOG = XmCreateToggleButton(parentform, "sono_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_SONO_TOG, defaults.sono_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Oscillo", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_OSC_TOG = XmCreateToggleButton(parentform, "osc_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_OSC_TOG, defaults.osc_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("PSTH", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_PSTH_TOG = XmCreateToggleButton(parentform, "psth_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_PSTH_TOG, defaults.psth_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Raster", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_RAST_TOG = XmCreateToggleButton(parentform, "raster_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_RAST_TOG, defaults.rast_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("PSIP", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_PSIP_TOG = XmCreateToggleButton(parentform, "psip_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_PSIP_TOG, defaults.psip_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Video", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_VIDEO_TOG = XmCreateToggleButton(parentform, "video_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_VIDEO_TOG, defaults.video_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("ISI", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_ISI_TOG = XmCreateToggleButton(parentform, "isi_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_ISI_TOG, defaults.isi_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("CCOR", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_CCOR_TOG = XmCreateToggleButton(parentform, "ccor_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_CCOR_TOG, defaults.ccor_selected, FALSE);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Label", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PLOT_LABEL_TOG = XmCreateToggleButton(parentform, "label_tog", args, n);
	XmStringFree(xmstr);
	XmToggleButtonSetState(WDG_PLOT_LABEL_TOG, defaults.label_selected, FALSE);

	/*
	** THE PANEL SECTION
	*/
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Panel", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PANEL_LBL = XmCreateLabel(parentform, "panel_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("New", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PANEL_NEW_BUT = XmCreatePushButton(parentform, "panel_new_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_PANEL_NEW_BUT, XmNactivateCallback, (XtCallbackProc) cb_panel_new_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Close", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PANEL_CLOSE_BUT = XmCreatePushButton(parentform, "panel_close_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_PANEL_CLOSE_BUT, XmNactivateCallback, (XtCallbackProc) cb_panel_close_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Print", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PANEL_PRINT_BUT = XmCreatePushButton(parentform, "panel_print_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_PANEL_PRINT_BUT, XmNactivateCallback, (XtCallbackProc) cb_panel_print_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Save", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PANEL_SAVE_BUT = XmCreatePushButton(parentform, "panel_save_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_PANEL_SAVE_BUT, XmNactivateCallback, (XtCallbackProc) cb_panel_save_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Play", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PANEL_PLAY_BUT = XmCreatePushButton(parentform, "panel_play_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_PANEL_PLAY_BUT, XmNactivateCallback, (XtCallbackProc) cb_panel_play_but, NULL);

	n = 0;
	XtSetArg(args[n], XmNlistSizePolicy, XmDYNAMIC); n++;
	XtSetArg(args[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
	XtSetArg(args[n], XmNwidth, 150); n++;
	WDG_PANEL_LIST = XmCreateScrolledList(parentform, "panel_list", args, n);
	XtAddCallback(WDG_PANEL_LIST, XmNextendedSelectionCallback, (XtCallbackProc) cb_panel_list, NULL);

	/*
	** THE PROP SECTION
	*/
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Trange(mem)", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PROP_X_LBL = XmCreateLabel(parentform, "prop_x_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNcolumns, 6); n++;
	WDG_PROP_XMIN_TEXT = XmCreateTextField(parentform, "prop_xmin", args, n);
	XtAddCallback(WDG_PROP_XMIN_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_XMIN_TEXT);
	WDG_PROP_XMAX_TEXT = XmCreateTextField(parentform, "prop_xmax", args, n);
	XtAddCallback(WDG_PROP_XMAX_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_XMAX_TEXT);
	
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Yrange", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PROP_Y_LBL = XmCreateLabel(parentform, "prop_y_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNcolumns, 6); n++;
	WDG_PROP_YMIN_TEXT = XmCreateTextField(parentform, "prop_ymin", args, n);
	XtAddCallback(WDG_PROP_YMIN_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_YMIN_TEXT);
	WDG_PROP_YMAX_TEXT = XmCreateTextField(parentform, "prop_ymax", args, n);
	XtAddCallback(WDG_PROP_YMAX_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_YMAX_TEXT);
	
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("FFTrange", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PROP_GS_LBL = XmCreateLabel(parentform, "gsrange_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNcolumns, 6); n++;
	WDG_PROP_GSMIN_TEXT = XmCreateTextField(parentform, "prop_gsmin", args, n);
	XtAddCallback(WDG_PROP_GSMIN_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_GSMIN_TEXT);
	WDG_PROP_GSMAX_TEXT = XmCreateTextField(parentform, "prop_gsmax", args, n);
	XtAddCallback(WDG_PROP_GSMAX_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_GSMAX_TEXT);
	
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("FRQrange", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PROP_FR_LBL = XmCreateLabel(parentform, "freqrange_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNcolumns, 6); n++;
	WDG_PROP_FRMIN_TEXT = XmCreateTextField(parentform, "prop_frmin", args, n);
	XtAddCallback(WDG_PROP_FRMIN_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_FRMIN_TEXT);
	WDG_PROP_FRMAX_TEXT = XmCreateTextField(parentform, "prop_frmax", args, n);
	XtAddCallback(WDG_PROP_FRMAX_TEXT, XmNactivateCallback, (XtCallbackProc) cb_range_text, (void *)WN_PROP_FRMAX_TEXT);
	
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Trange(vis)", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_PROP_XRANGE_LBL = XmCreateLabel(parentform, "prop_xrange_lbl", args, n);
	XmStringFree(xmstr);

	n = 0;
	XtSetArg(args[n], XmNcolumns, 6); n++;
	WDG_PROP_XRANGEMIN_TEXT = XmCreateTextField(parentform, "prop_xrangemin", args, n);
	XtAddCallback(WDG_PROP_XRANGEMIN_TEXT, XmNactivateCallback, (XtCallbackProc) cb_prop_zoom_but, (void *)WN_PROP_XRANGEMIN_TEXT);
	WDG_PROP_XRANGEMAX_TEXT = XmCreateTextField(parentform, "prop_xrangemax", args, n);
	XtAddCallback(WDG_PROP_XRANGEMAX_TEXT, XmNactivateCallback, (XtCallbackProc) cb_prop_zoom_but, (void *)WN_PROP_XRANGEMAX_TEXT);

	/*
	** THE BOTTOMMOST SECTION
	*/
	n = 0;
	XtSetArg(args[n], XmNlabelString, xmstr = XmStringGenerate("Quit", XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL)); n++;
	WDG_QUIT_BUT = XmCreatePushButton(parentform, "quit_but", args, n);
	XmStringFree(xmstr);
	XtAddCallback(WDG_QUIT_BUT, XmNactivateCallback, (XtCallbackProc) cb_quit_but, NULL);

	/*
	** Set the geometries within the form widget
	*/

	/* The QUIT area doesn't depend on anything */
	xm_attach(WDG_QUIT_BUT, "nnff", 10, 10);

	xm_attach(WDG_PROP_XRANGE_LBL, "nfnf", 0, 10);
	xm_attach(WDG_PROP_XRANGEMIN_TEXT, "nwnW", WDG_PROP_XRANGE_LBL, 0, WDG_PROP_XRANGE_LBL, 0);
	xm_attach(WDG_PROP_XRANGEMAX_TEXT, "WwnW", WDG_PROP_XRANGEMIN_TEXT, 0, WDG_PROP_XRANGEMIN_TEXT, 0, WDG_PROP_XRANGEMIN_TEXT, 0);

	xm_attach(WDG_PROP_FR_LBL, "WwnW", WDG_PROP_XRANGEMAX_TEXT, 0, WDG_PROP_XRANGEMAX_TEXT, 0, WDG_PROP_XRANGEMAX_TEXT, 0);
	xm_attach(WDG_PROP_FRMIN_TEXT, "nwnW", WDG_PROP_FR_LBL, 0, WDG_PROP_XRANGEMAX_TEXT, 0);
	xm_attach(WDG_PROP_FRMAX_TEXT, "nwnW", WDG_PROP_FRMIN_TEXT, 0, WDG_PROP_FRMIN_TEXT, 0);

	xm_attach(WDG_PROP_XMAX_TEXT, "nWWw", WDG_PROP_XRANGEMAX_TEXT, 0, WDG_PROP_XRANGEMAX_TEXT, 0, WDG_PROP_XRANGEMAX_TEXT, 0);
	xm_attach(WDG_PROP_XMIN_TEXT, "WnwW", WDG_PROP_XMAX_TEXT, 0, WDG_PROP_XMAX_TEXT, 0, WDG_PROP_XMAX_TEXT, 0);
	xm_attach(WDG_PROP_X_LBL, "WnwW", WDG_PROP_XMIN_TEXT, 0, WDG_PROP_XMIN_TEXT, 0, WDG_PROP_XMIN_TEXT, 0);

	xm_attach(WDG_PROP_GS_LBL, "WwnW", WDG_PROP_X_LBL, 0, WDG_PROP_XMAX_TEXT, 0, WDG_PROP_X_LBL, 0);
	xm_attach(WDG_PROP_GSMIN_TEXT, "WwnW", WDG_PROP_GS_LBL, 0, WDG_PROP_GS_LBL, 0, WDG_PROP_GS_LBL, 0);
	xm_attach(WDG_PROP_GSMAX_TEXT, "nwnW", WDG_PROP_GSMIN_TEXT, 0, WDG_PROP_GSMIN_TEXT, 0);

	xm_attach(WDG_PROP_Y_LBL, "WwnW", WDG_PROP_GS_LBL, 0, WDG_PROP_GSMAX_TEXT, 0, WDG_PROP_GS_LBL, 0);
	xm_attach(WDG_PROP_YMIN_TEXT, "WwnW", WDG_PROP_Y_LBL, 0, WDG_PROP_Y_LBL, 0, WDG_PROP_Y_LBL, 0);
	xm_attach(WDG_PROP_YMAX_TEXT, "nwnW", WDG_PROP_YMIN_TEXT, 0, WDG_PROP_YMIN_TEXT, 0);

	xm_attach(WDG_SEP1, "nffw", 0, 0, WDG_PROP_XMIN_TEXT, 10);

	/* The FILE area depends on the PROP area for bottompos */
	xm_attach(WDG_FILE_LBL, "ffpn", 0, 0, 40);
	xm_attach(XtParent(WDG_FILE_LIST), "wfpw", WDG_FILE_LBL, 0, 0, 40, WDG_FILE_ADD_BUT, 0);
	xm_attach(WDG_FILE_ADD_BUT, "nfnw", 0, WDG_FILE_REWIND_BUT, 0);
	xm_attach(WDG_FILE_DROP_BUT, "WwnW", WDG_FILE_ADD_BUT, 0, WDG_FILE_ADD_BUT, 0, WDG_FILE_ADD_BUT, 0);

	xm_attach(WDG_FILE_REWIND_BUT, "nfnw", 0, WDG_SEP1, 10);
	xm_attach(WDG_FILE_PREV_BUT, "WwnW", WDG_FILE_REWIND_BUT, 0, WDG_FILE_REWIND_BUT, 0, WDG_FILE_REWIND_BUT, 0);
	xm_attach(WDG_FILE_NEXT_BUT, "WwnW", WDG_FILE_PREV_BUT, 0, WDG_FILE_PREV_BUT, 0, WDG_FILE_PREV_BUT, 0);
	xm_attach(WDG_FILE_ENTRY_TEXT, "nwnW", WDG_FILE_NEXT_BUT, 0, WDG_FILE_NEXT_BUT, 0);

	/* The PLOT area depends only on the FILE_LIST for leftpos, height */
	xm_attach(WDG_PLOT_SONO_TOG, "Wwnn", XtParent(WDG_FILE_LIST), 0, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_OSC_TOG, "wwnn", WDG_PLOT_SONO_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_LABEL_TOG, "wwnn", WDG_PLOT_OSC_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_PSTH_TOG, "wwnn", WDG_PLOT_LABEL_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_RAST_TOG, "wwnn", WDG_PLOT_PSTH_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_ISI_TOG, "wwnn", WDG_PLOT_RAST_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_CCOR_TOG, "wwnn", WDG_PLOT_ISI_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_PSIP_TOG, "wwnn", WDG_PLOT_CCOR_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_VIDEO_TOG, "wwnn", WDG_PLOT_PSIP_TOG, -5, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_DROP_BUT, "nwnW", WDG_PLOT_OSC_TOG, 0, XtParent(WDG_FILE_LIST), 0);
	xm_attach(WDG_PLOT_ADD_BUT, "nWWw", WDG_PLOT_DROP_BUT, 0, WDG_PLOT_DROP_BUT, 0, WDG_PLOT_DROP_BUT, 0);
	xm_attach(WDG_PLOT_LBL, "fpWn", 0, 40, WDG_PLOT_DROP_BUT, 0);

	/* The PANEL area depends on PLOT_DROP_BUT for leftpos, height */
	xm_attach(WDG_PANEL_LBL, "fwfn", 0, WDG_PLOT_DROP_BUT, 0, 0);
	xm_attach(XtParent(WDG_PANEL_LIST), "wwfW", WDG_PANEL_LBL, 0, WDG_PLOT_DROP_BUT, 0, 0, WDG_PLOT_DROP_BUT, 0);
	xm_attach(WDG_PANEL_NEW_BUT, "wWnn", XtParent(WDG_PANEL_LIST), 0, XtParent(WDG_PANEL_LIST), 0);
	xm_attach(WDG_PANEL_CLOSE_BUT, "wwnn", XtParent(WDG_PANEL_LIST), 0, WDG_PANEL_NEW_BUT, 0);
	xm_attach(WDG_PANEL_PRINT_BUT, "wwnn", XtParent(WDG_PANEL_LIST), 0, WDG_PANEL_CLOSE_BUT, 0);
	xm_attach(WDG_PANEL_SAVE_BUT, "wwnn", XtParent(WDG_PANEL_LIST), 0, WDG_PANEL_PRINT_BUT, 0);
	xm_attach(WDG_PANEL_PLAY_BUT, "wwnn", XtParent(WDG_PANEL_LIST), 0, WDG_PANEL_SAVE_BUT, 0);

	XtManageChild(WDG_FILE_LIST);
	XtManageChildren(children, WN_NUMCHILDREN);
	XtManageChild(WDG_PANEL_LIST);
	XtManageChild(parentform);
}


static int panlistsize = 0;
static char *(panlist[100]);
static int panlistselected[100];

static void list_add_item(char *entry, int pos, int selected)
{
	XmString xmstr;
	int i;

	if (pos == 0)
	{
		pos--;
		panlist[panlistsize] = strdup(entry);
		panlistselected[panlistsize++] = selected;
	}
	else
	{
		pos--;
		for (i=panlistsize; i > pos; i--)
		{
			panlist[i] = panlist[i-1];
			panlistselected[i] = panlistselected[i-1];
		}
		panlist[pos] = strdup(entry);
		panlistselected[pos] = selected;
		panlistsize++;
	}
	xmstr = XmStringGenerate(entry, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL);
	XmListAddItemUnselected(WDG_PANEL_LIST, xmstr, pos + 1);
	if (selected != 0)
		XmListSelectItem(WDG_PANEL_LIST, xmstr, TRUE);
#if XmVersion > 1001
	XmListUpdateSelectedList(WDG_PANEL_LIST);
#endif
	XmStringFree(xmstr);
}

static void list_drop_itempos(int pos)
{
	int i;

	pos--;
	free(panlist[pos]);
	for (i=pos; i < panlistsize - 1; i++)
	{
		panlist[i] = panlist[i+1];
		panlistselected[i] = panlistselected[i+1];
	}
	panlistsize--;
	XmListDeletePos(WDG_PANEL_LIST, pos + 1);
}

static void list_drop_itemspos(int count, int pos)
{
	int i;

	pos--;
	for (i=pos; i < pos+count; i++)
		free(panlist[i]);
	for (i=pos; i < panlistsize - count; i++)
	{
		panlist[i] = panlist[i+count];
		panlistselected[i] = panlistselected[i+count];
	}
	panlistsize -= count;
	XmListDeleteItemsPos(WDG_PANEL_LIST, count, pos + 1);
}

#ifdef notdef
static void list_selectitem(char *item)
{
	int i;
	XmString xmstr;

	for (i=0; i < panlistsize; i++)
	{
		if (!strcmp(item, panlist[i]))
		{
			panlistselected[i] = 1;
			break;
		}
	}
	xmstr = XmStringGenerate(item, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, NULL);
	XmListSelectItem(WDG_PANEL_LIST, xmstr, TRUE);
	XmStringFree(xmstr);
}
#endif

static void list_updateselected(int selected_item_count, int *selected_positions)
{
	int i;

	for (i=0; i < panlistsize; i++)
		panlistselected[i] = 0;
	for (i=0; i < selected_item_count; i++)
		panlistselected[ selected_positions[i] - 1 ] = 1;
}

/***********************************************************************************************************/
/********************                                                            ***************************/
/********************                       U T I L I T Y                        ***************************/
/********************                                                            ***************************/
/***********************************************************************************************************/
static void update_selection_tree(void)
{
	PANEL *p, *cur_selpanel;
	GROUP *g, *cur_selgroup;
	char **list_arr, *buf;
	int *sel_arr, arrsize, i, j, groupnum, panpos;

	/*
	** The selection tree is a sub-tree of the panels/groups tree.
	** The top level pointer is selpanel.  This function queries the WDG_PANEL_LIST, and based
	** on the string contents of the selected list items, creates a subtree within the panels
	** tree, and sets selpanel to point to it.
	** If a panel is selected, then all groups in that panel are automatically selected.
	** The selection subtree uses the *next_sel and *groups_sel fields of the PANEL and
	** GROUP structures.
	*/

	/*
	** Clear the selection tree fields...
	*/
	selpanel = NULL;
	for (p = panels; p != NULL; p = p->next)
	{
		for (g = p->groups; g != NULL; g = g->next)
		{
			g->next_sel = NULL;
			g->listpos = 0;
			g->selected = FALSE;
		}
		p->next_sel = NULL;
		p->groups_sel = NULL;
		p->listpos = 0;
		p->selected = FALSE;
	}

	/*
	** Create duplicates of the panlist arrays
	*/
	arrsize = panlistsize;
	list_arr = (char **)calloc(1 + arrsize, sizeof(char *));
	sel_arr = (int *)calloc(1 + arrsize, sizeof(int));
	for (i=1; i <= arrsize; i++)
	{
		list_arr[i] = panlist[i-1];
		sel_arr[i] = panlistselected[i-1];
	}

	/* If a panel is selected, select all of its groups */
	for (i=1; i <= arrsize; i++)
	{
		buf = list_arr[i];
		if ((sel_arr[i] != 0) && (buf[0] == 'P'))
		{
			for (j = i+1; j <= arrsize; j++)
			{
				buf = list_arr[j];
				if (buf[0] == 'P')
					break;
				sel_arr[j] = 1;
			}
		}
	}

	/* If a group is selected, select its panel */
	/* Also, for each panel, set the panpos field of the PANEL to the panel's line number in the list */
	panpos = 0;
	cur_selpanel = NULL;
	for (i=1; i <= arrsize; i++)
	{
		buf = list_arr[i];
		if (buf[0] == 'P')
		{
			/* This is a panel.  store its position, and note whether it is selected or not */
			panpos = i;
			for (p = panels; p != NULL; p = p->next)
			{
				if (!strcmp(p->name, buf))
				{
					p->listpos = panpos;
					cur_selpanel = p;
					break;
				}
			}
		}
		else
		{
			/* This is a group.  store its position */
			if (sscanf(buf, "    Group %d", &groupnum) == 1)
			{
				for (g = cur_selpanel->groups; g != NULL; g = g->next)
					if (g->groupnum == groupnum)
					{
						g->listpos = i;
						break;
					}
			}
			/* If we are selected and our panel isn't, then select the panel */
			if (sel_arr[i] && (sel_arr[panpos] == 0))
				sel_arr[panpos] = 1;
		}
	}

	/* Now, we can fairly easily decide who's selected */
	cur_selpanel = NULL;
	cur_selgroup = NULL;
	for (i=1; i <= arrsize; i++)
	{
		if (sel_arr[i] != 0)
		{
			buf = list_arr[i];
			if (buf[0] == 'P')
			{
				/* This is a panel.  Find out which panel it is, and add it to the selection tree */
				for (p = panels; p != NULL; p = p->next)
					if (!strcmp(p->name, buf))
					{
						if (selpanel == NULL)
							selpanel = p;
						if (cur_selpanel != NULL)
							cur_selpanel->next_sel = p;
						cur_selpanel = p;
						cur_selpanel->selected = TRUE;
						cur_selpanel->groups_sel = NULL;
						cur_selpanel->next_sel = NULL;
						cur_selgroup = NULL;
						break;
					}
			}
			else
			{
				/* This is a group.  Find out which group it is, and add it to the selection tree */
				if ((cur_selpanel != NULL) && (sscanf(buf, "    Group %d", &groupnum) == 1))
				{
					for (g = cur_selpanel->groups; g != NULL; g = g->next)
						if (g->groupnum == groupnum)
						{
							if (cur_selpanel->groups_sel == NULL)
								cur_selpanel->groups_sel = g;
							if (cur_selgroup != NULL)
								cur_selgroup->next_sel = g;
							cur_selgroup = g;
							cur_selgroup->selected = TRUE;
							cur_selgroup->next_sel = NULL;
							break;
						}
				}
			}
		}
	}

	/*
	** Free the duplicates
	*/
	free(list_arr);
	free(sel_arr);

	return;
}


/***********************************************************************************************************/
/********************                                                            ***************************/
/********************                     C A L L B A C K S                      ***************************/
/********************                                                            ***************************/
/***********************************************************************************************************/
static void cb_file_dialog_ok(Widget w, void *data, XmFileSelectionBoxCallbackStruct *call_data)
{
	XmListAddItem(WDG_FILE_LIST, (XmString)call_data->value, 0);
	XmListDeselectAllItems(WDG_FILE_LIST);
	XmListSelectItem(WDG_FILE_LIST, (XmString)call_data->value, TRUE);
#if XmVersion > 1001
	XmListUpdateSelectedList(WDG_FILE_LIST);
#endif
	XtUnmanageChild(w);
}


static void cb_file_dialog_cancel(Widget w, void *data, XtPointer call_data)
{
	XtUnmanageChild(w);
}

static void cb_file_add_but(Widget w, void *data, XtPointer call_data)
{
	XtManageChild(WDG_FILE_DIALOG);
}

static void cb_file_drop_but(Widget w, void *data, XtPointer call_data)
{
	XmString *list_xmstrings;
	int num_list_xmstrings;

	XtVaGetValues(WDG_FILE_LIST,
		XmNselectedItems, &list_xmstrings,
		XmNselectedItemCount, &num_list_xmstrings, NULL);
	if (num_list_xmstrings > 0)
		XmListDeleteItems(WDG_FILE_LIST, list_xmstrings, num_list_xmstrings);
#if XmVersion > 1001
	XmListUpdateSelectedList(WDG_FILE_LIST);
#endif
}

void change_file_entry(int direction)
{
	PANEL *pan;
	GROUP *group;
	int entry, newentry;
	char *curentrystring, newentrystring[100];

	curentrystring = XmTextFieldGetString(WDG_FILE_ENTRY_TEXT);
	if (sscanf(curentrystring, "%d", &entry) != 1) entry = 1;
	XtFree(curentrystring);
	newentry = entry;

	switch (direction)
	{
		case -1:
			if (--newentry < 1)
				newentry = 1;
			break;
		case 1:
			newentry++;
			break;
		default:
			break;
	}

	sprintf(newentrystring, "%d", newentry);
	XmTextFieldSetString(WDG_FILE_ENTRY_TEXT, newentrystring);

	/* For every selected group... */
	update_selection_tree();
	for (pan = selpanel; pan != NULL; pan = pan->next_sel)
	{
		for (group = pan->groups_sel; group; group = group->next_sel)
		{
			if (group->entry != newentry)
				group->dirty = 1;
			group->entry = newentry;
			group->zoomhist_count = 0;
			group->xrangemin = 0;
			group->xrangemax = 0;
		}
		panel_display(pan);
	}
	update_fields(selpanel);
}

static void cb_file_entry_changed(Widget w, int data, XtPointer call_data)
{
	PANEL *pan;
	GROUP *group;
	int entry, newentry;
	char *curentrystring, newentrystring[100];

	curentrystring = XmTextFieldGetString(WDG_FILE_ENTRY_TEXT);
	if (sscanf(curentrystring, "%d", &entry) != 1) entry = 1;
	XtFree(curentrystring);
	newentry = entry;

	switch (data)
	{
		case WN_FILE_REWIND_BUT:
			newentry = 1;
			break;
		case WN_FILE_PREV_BUT:
			if (--newentry < 1)
				newentry = 1;
			break;
		case WN_FILE_NEXT_BUT:
			newentry++;
			break;
		case WN_FILE_ENTRY_TEXT:
			break;
	}

	sprintf(newentrystring, "%d", newentry);
	XmTextFieldSetString(WDG_FILE_ENTRY_TEXT, newentrystring);

	update_selection_tree();
	for (pan = selpanel; pan != NULL; pan = pan->next_sel)
	{
		for (group = pan->groups_sel; group; group = group->next_sel)
		{
			if (group->entry != newentry)
				group->dirty = 1;
			group->entry = newentry;
			group->zoomhist_count = 0;
			group->xrangemin = 0;
			group->xrangemax = 0;
		}
		panel_display(pan);
	}
	update_fields(selpanel);
}

static void cb_file_list(Widget w, XtPointer clientData, XtPointer callData)
{
}

static void cb_plot_add_but(Widget w, void *data, XtPointer call_data)
{
	PANEL *pan;
	GROUP *group = NULL;
	char **selectedfiles;
	int numselectedfiles, entry;
	char types[132], *curentrystring;
	XmStringTable list_xmstrings;
	int num_list_xmstrings, i, doubleflag;

	/* What files are selected? */
	num_list_xmstrings = 0;
	XtVaGetValues(WDG_FILE_LIST, XmNselectedItems, &list_xmstrings, XmNselectedItemCount, &num_list_xmstrings, NULL);
	numselectedfiles = num_list_xmstrings;
	if (numselectedfiles == 0)
		return;
	selectedfiles = (char **)XmStringTableUnparse(list_xmstrings, num_list_xmstrings, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, 0);

	/* Use the current entry */
	curentrystring = XmTextFieldGetString(WDG_FILE_ENTRY_TEXT);
	if (sscanf(curentrystring, "%d", &entry) != 1) entry = 1;
	XtFree(curentrystring);

	/* What plots are selected? */
	doubleflag = 0;
	types[0] = '\0';
	if (XmToggleButtonGetState(WDG_PLOT_SONO_TOG)) strcat(types, "sono ");
	if (XmToggleButtonGetState(WDG_PLOT_OSC_TOG))  strcat(types, "osc ");
	if (XmToggleButtonGetState(WDG_PLOT_PSTH_TOG)) strcat(types, "psth ");
	if (XmToggleButtonGetState(WDG_PLOT_RAST_TOG)) strcat(types, "rast ");
	if (XmToggleButtonGetState(WDG_PLOT_PSIP_TOG)) strcat(types, "psip ");
	if (XmToggleButtonGetState(WDG_PLOT_VIDEO_TOG)) strcat(types, "video ");
	if (XmToggleButtonGetState(WDG_PLOT_ISI_TOG)) strcat(types, "isi ");
	if (XmToggleButtonGetState(WDG_PLOT_CCOR_TOG)) doubleflag = 1;
	if (XmToggleButtonGetState(WDG_PLOT_LABEL_TOG)) strcat(types, "label ");
	if ((types[0] != '\0') || (doubleflag != 0))
	{
		/*
		** What is the selected panel?
		** If there's no selected panel, try to add one.
		** Also, update_selection_tree() sets the listpos field of the panel, which we'll need below.
		*/
		update_selection_tree();
		if (selpanel == NULL)
		{
			cb_panel_new_but((Widget)NULL, (void *)NULL, (XtPointer)NULL);
			update_selection_tree();
		}
		pan = selpanel;

		if (pan != NULL)
		{
			/*
			** If more than one file, check if any of the plot types can use them all
			** Otherwise, loop through the files, calling panel_addplotgroup() for each one
			** For each success, add a line to the panel listview
			*/
			if (types[0] != '\0')
			{
				for (i=0; i < numselectedfiles; i++)
				{
					/*
					** This function may return an error if a plot failed to open.  Even if that
					** is the case, it may still have worked (because other plots did open).
					** Thus, check for group != NULL, and ignore the return code...
					*/
					panel_addplotgroup(pan, selectedfiles[i], entry, types, &group, NULL);
					if (group != NULL)
					{
						sprintf(group->group_panellistline, "    Group %d: %s %d %s", group->groupnum,
							mybasename(selectedfiles[i]), entry, group->group_plottypes);
						list_add_item(group->group_panellistline, pan->listpos + pan->numgroups, 1);
						group->dirty = 1;
					}
				}
			}
			if ((doubleflag) && (numselectedfiles == 2))
			{
				types[0] = '\0';
				if (XmToggleButtonGetState(WDG_PLOT_CCOR_TOG)) strcat(types, "ccor ");

				panel_addplotgroup(pan, selectedfiles[0], entry, types, &group, selectedfiles[1]);
				if (group != NULL)
				{
					sprintf(group->group_panellistline, "    Group %d: %s %d %s", group->groupnum,
						mybasename(selectedfiles[0]), entry, group->group_plottypes);
					list_add_item(group->group_panellistline, pan->listpos + pan->numgroups, 1);
					group->dirty = 1;
					i = 2;
				}
			}
			panel_display(pan);
			update_fields(pan);
		}
	}

	/* Free the list of filenames */
	for (i=0; i < numselectedfiles; i++)
		XtFree(selectedfiles[i]);
	free(selectedfiles);
}

static void cb_plot_drop_but(Widget w, void *data, XtPointer call_data)
{
	PANEL *pan;

	update_selection_tree();
	for (pan = selpanel; pan != NULL; pan = pan->next_sel)
	{
		if (pan->groups_sel != NULL)
		{
			if ((pan->groups->next == NULL) && (pan->groups == pan->groups_sel))
				cb_panel_close_but(w, data, call_data);
			else
			{
				list_drop_itempos(pan->groups_sel->listpos);
				panel_deleteplotgroup(pan, pan->groups_sel->groupnum);
			}
		}
		break;														/* I only want to delete the groups one by one for now */
	}
}


static void cb_panel_new_but(Widget w, void *data, XtPointer call_data)
{
	PANEL *new, *travel;
	int num = 1;

	if ((new = (PANEL *)calloc(sizeof(PANEL), 1)) != NULL)
	{
		if (panels == NULL)
			panels = new;
		else if (panels->next == NULL)
		{
			panels->next = new;
			if (panels->num == num)
				num++;
		}
		else
		{
			travel = panels;
			for (;;)
			{
				if (travel->num == num)
				{
					num++;
					travel = panels;
					continue;
				}
				if (travel->next == NULL)
					break;
				travel = travel->next;
			}
			travel->next = new;
		}

		new->next = NULL;
		new->num = num;
		sprintf(new->name, "Panel %d", num);			/* CAREFUL - Do not change */
		new->groups = NULL;
		new->numplots = 0;
		new->realized = FALSE;

		panel_open(new);
		XmAddWMProtocolCallback(new->panel_shell, WM_DELETE_WINDOW, cb_panel_close_but, new);
		list_add_item(new->name, 0, 1);
		update_selection_tree();
	}
}

static void cb_panel_close_but(Widget w, void *data, XtPointer call_data)
{
	PANEL *pan, *travel;
	int pos, numgroups;

	for(;;)
	{
		update_selection_tree();

		/*
		** If we got called from the Window manager, the client_data
		** identifies the panel to be closed.  Otherwise, search the
		** panels list for selected panels, and close them all, one by one.
		*/
		if (data != NULL)
			pan = (PANEL *)data;
		else
		{
			if ((pan = selpanel) == NULL)
				break;
		}

		numgroups = pan->numgroups;
		pos = pan->listpos;

		/* Remove the panel from the panel list */
		list_drop_itemspos(numgroups+1, pos);

		/* Remove the panel from the PANEL linked list */
		travel = panels;
		if (travel)
		{
			if (travel == pan)
				panels = pan->next;
			else
			{
				while (travel->next != pan)
					travel = travel->next;
				travel->next = pan->next;
			}
		}

		/* Do NOT free the PANEL structure yet! */
		panel_close(pan);

		/* Free the PANEL struct from memory */
		free(pan);
		pan = NULL;

		update_selection_tree();
		update_fields(selpanel);

		/*
		** If we've been called from the window manager, (the user pressed the quit button
		** or pressed Alt-F4, then we aren't searching the panels list, and we only get rid
		** of this one panel
		*/
		if (data != NULL)	
			break;
	}
}

static void cb_panel_print_but(Widget w, void *data, XtPointer call_data)
{
	XmPushButtonCallbackStruct *xcb = (XmPushButtonCallbackStruct *)call_data;
	XButtonEvent *xev;
	PANEL *pan;
	int printtype;
	char *filename;
	static Widget dialog = NULL;

	printtype = PRINT_TO_LPR;									/* By default, print to printer */
	filename = NULL;
	if (xcb != NULL) 
	{
		xev = (XButtonEvent *)xcb->event;
		if ((xev->type == ButtonPress) || (xev->type == ButtonRelease))
		{
			if ((xev->state & ShiftMask) && (xev->state & ControlMask))
				printtype = PRINT_TO_XFIG;						/* print indiv. EPS files + XFIG file */
			if ((xev->state & ShiftMask) == 1)
				printtype = PRINT_TO_SINGLE;					/* print to single output file */
			else if ((xev->state & Mod1Mask) == 1)
				printtype = PRINT_TO_MANY;						/* print individual EPS files */
			else
				printtype = PRINT_TO_LPR;						/* print to printer */
		}
	}

	if ((printtype == PRINT_TO_SINGLE) || (printtype == PRINT_TO_XFIG))
	{
		if (dialog == NULL)
		{
			dialog = XmCreateFileSelectionDialog(toplevel, "psfile_dialog", NULL, 0);
			XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
			XtSetSensitive(dialog, True);
			XtAddCallback(dialog, XmNokCallback, (XtCallbackProc) cb_panel_psfile_dialog_ok, NULL);
			XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc) cb_panel_psfile_dialog_cancel, NULL);
		}
		XtManageChild(dialog);
	}
	else
	{
		update_selection_tree();
		if ((pan = selpanel) != NULL)
			panel_print(pan, printtype, filename);
	}
}

static void cb_panel_psfile_dialog_ok(Widget w, void *data, XmFileSelectionBoxCallbackStruct *call_data)
{
	PANEL *pan;
	char *filename;

	filename = (char *)XmStringUnparse((XmString)call_data->value, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, 0);
	update_selection_tree();
	if ((pan = selpanel) != NULL)
	{
		if (!strcmp(filename + strlen(filename) - 4, ".fig"))
			panel_print(pan, PRINT_TO_XFIG, filename);
		else
			panel_print(pan, PRINT_TO_SINGLE, filename);
	}
	XtFree(filename);
	XtUnmanageChild(w);
}

static void cb_panel_psfile_dialog_cancel(Widget w, void *data, XtPointer call_data)
{
	XtUnmanageChild(w);
}

static void cb_panel_save_but(Widget w, void *data, XtPointer call_data)
{
	PANEL *pan;

	update_selection_tree();
	if ((pan = selpanel) != NULL)
		panel_save(pan);
}


static void cb_panel_play_but(Widget w, void *data, XtPointer call_data)
{
	PANEL *pan;

	update_selection_tree();
	if ((pan = selpanel) != NULL)
		panel_play(pan);
}


static void cb_prop_zoom_but(Widget w, void *data, XtPointer call_data)
{
	PANEL *pan;
	float xrangemin, xrangemax;
	char *tmpstr;

	if (((tmpstr = XmTextFieldGetString(WDG_PROP_XRANGEMIN_TEXT)) != NULL) && (*tmpstr != '\0'))
		sscanf(tmpstr, "%f", &xrangemin);
	if (tmpstr) XtFree(tmpstr);
	if (((tmpstr = XmTextFieldGetString(WDG_PROP_XRANGEMAX_TEXT)) != NULL) && (*tmpstr != '\0'))
		sscanf(tmpstr, "%f", &xrangemax);
	if (tmpstr) XtFree(tmpstr);

	update_selection_tree();
	for (pan = selpanel; pan != NULL; pan = pan->next_sel)
		panel_zoom(pan, xrangemin, xrangemax);
}

static void cb_quit_but(Widget w, void *data, XtPointer call_data)
{
	XmPushButtonCallbackStruct *xcb = (XmPushButtonCallbackStruct *)call_data;
	XButtonEvent *xev;
	int defsect;

	if (xcb != NULL) 
	{
		xev = (XButtonEvent *)xcb->event;
		if ((xev->type == ButtonPress) || (xev->type == ButtonRelease))
		{
			if (xev->state & ShiftMask)
			{
				/* Save our preferences to the defaults section designated on startup */
				defsect = resource_data.defaults_section;
				save_defaults(defsect);
			}
		}
	}

	XtDestroyApplicationContext(app_con);
	exit(1);
}

void ui_get_gs_range(int *gsmin_p, int *gsmax_p)
{
	char *tmpstr = NULL;
	int gsmin = 0, gsmax = 0;

	tmpstr = XmTextFieldGetString(WDG_PROP_GSMIN_TEXT);
	if ((tmpstr != NULL) && (*tmpstr != '\0'))
		gsmin = strtol(tmpstr, NULL, 10);
	if (tmpstr) XtFree(tmpstr);
	tmpstr = XmTextFieldGetString(WDG_PROP_GSMAX_TEXT);
	if ((tmpstr != NULL) && (*tmpstr != '\0'))
		gsmax = strtol(tmpstr, NULL, 10);
	if (tmpstr) XtFree(tmpstr);
	*gsmin_p = gsmin;
	*gsmax_p = gsmax;
}

void ui_set_gs_range(int gsmin, int gsmax)
{
	char tmpstr[80];

	tmpstr[0] = '\0';
	snprintf(tmpstr, sizeof(tmpstr), "%d", gsmin);
	XmTextFieldSetString(WDG_PROP_GSMIN_TEXT, tmpstr);
	snprintf(tmpstr, sizeof(tmpstr), "%d", gsmax);
	XmTextFieldSetString(WDG_PROP_GSMAX_TEXT, tmpstr);
}

static void cb_range_text(Widget w, void *data, XtPointer call_data)
{
	char *tmpstr;
	PANEL *pan;
	GROUP *group;
	int gsmin, gsmax, ymin, ymax, frmin, frmax;

	update_selection_tree();

	ymin = ymax = gsmin = gsmax = frmin = frmax = 0;
	if (((tmpstr = XmTextFieldGetString(WDG_PROP_YMIN_TEXT)) != NULL) && (*tmpstr != '\0'))
	{
		sscanf(tmpstr, "%d", &ymin);
		if (selpanel == NULL) defaults.ymin = ymin;
	}
	if (tmpstr) XtFree(tmpstr);

	if (((tmpstr = XmTextFieldGetString(WDG_PROP_YMAX_TEXT)) != NULL) && (*tmpstr != '\0'))
	{
		sscanf(tmpstr, "%d", &ymax);
		if (selpanel == NULL) defaults.ymax = ymax;
	}
	if (tmpstr) XtFree(tmpstr);

	if (((tmpstr = XmTextFieldGetString(WDG_PROP_GSMIN_TEXT)) != NULL) && (*tmpstr != '\0'))
	{
		sscanf(tmpstr, "%d", &gsmin);
		defaults.gsmin = gsmin;
	}
	if (tmpstr) XtFree(tmpstr);

	if (((tmpstr = XmTextFieldGetString(WDG_PROP_GSMAX_TEXT)) != NULL) && (*tmpstr != '\0'))
	{
		sscanf(tmpstr, "%d", &gsmax);
		defaults.gsmax = gsmax;
	}
	if (tmpstr) XtFree(tmpstr);

	if (((tmpstr = XmTextFieldGetString(WDG_PROP_FRMIN_TEXT)) != NULL) && (*tmpstr != '\0'))
	{
		sscanf(tmpstr, "%d", &frmin);
		defaults.frmin = frmin;
	}
	if (tmpstr) XtFree(tmpstr);

	if (((tmpstr = XmTextFieldGetString(WDG_PROP_FRMAX_TEXT)) != NULL) && (*tmpstr != '\0'))
	{
		sscanf(tmpstr, "%d", &frmax);
		defaults.frmax = frmax;
	}
	if (tmpstr) XtFree(tmpstr);

	for (pan = selpanel; pan != NULL; pan = pan->next_sel)
	{
		for (group = pan->groups_sel; group; group = group->next_sel)
		{
			group->frmin = frmin;
			group->frmax = frmax;
			group->gsmin = gsmin;
			group->gsmax = gsmax;
			group->ymin = ymin;
			group->ymax = ymax;
		}
		panel_set(pan, NULL, 0);
	}
	update_fields(selpanel);
}

static void cb_panel_list(Widget w, XtPointer clientData, XtPointer callData)
{
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)callData;

	list_updateselected(cbs->selected_item_count, cbs->selected_item_positions);

	update_selection_tree();
	if (cbs->selected_item_count > 0)
		update_fields(selpanel);
	else if (cbs->selected_item_count == 0)
		update_fields(NULL);
}

void update_fields(PANEL *pan)
{
	GROUP *group;
	char tmpstr[132];

	if (pan != NULL)
	{
		for (group = pan->groups_sel; group != NULL; group = group->next_sel)
		{
			sprintf(tmpstr, "%.2f", group->start); XmTextFieldSetString(WDG_PROP_XMIN_TEXT, tmpstr);
			sprintf(tmpstr, "%.2f", group->stop); XmTextFieldSetString(WDG_PROP_XMAX_TEXT, tmpstr);
			sprintf(tmpstr, "%ld", group->ymin); XmTextFieldSetString(WDG_PROP_YMIN_TEXT, tmpstr);
			sprintf(tmpstr, "%ld", group->ymax); XmTextFieldSetString(WDG_PROP_YMAX_TEXT, tmpstr);
			sprintf(tmpstr, "%ld", group->gsmin); XmTextFieldSetString(WDG_PROP_GSMIN_TEXT, tmpstr);
			sprintf(tmpstr, "%ld", group->gsmax); XmTextFieldSetString(WDG_PROP_GSMAX_TEXT, tmpstr);
			sprintf(tmpstr, "%ld", group->frmin); XmTextFieldSetString(WDG_PROP_FRMIN_TEXT, tmpstr);
			sprintf(tmpstr, "%ld", group->frmax); XmTextFieldSetString(WDG_PROP_FRMAX_TEXT, tmpstr);
			sprintf(tmpstr, "%.2f", group->xrangemin); XmTextFieldSetString(WDG_PROP_XRANGEMIN_TEXT, tmpstr);
			sprintf(tmpstr, "%.2f", group->xrangemax); XmTextFieldSetString(WDG_PROP_XRANGEMAX_TEXT, tmpstr);
			sprintf(tmpstr, "%ld", group->entry); XmTextFieldSetString(WDG_FILE_ENTRY_TEXT, tmpstr);
		}
	}
	else
	{
		/* Show defaults... */
		sprintf(tmpstr, "%ld", defaults.xmin); XmTextFieldSetString(WDG_PROP_XMIN_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.xmax); XmTextFieldSetString(WDG_PROP_XMAX_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.ymin); XmTextFieldSetString(WDG_PROP_YMIN_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.ymax); XmTextFieldSetString(WDG_PROP_YMAX_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.gsmin); XmTextFieldSetString(WDG_PROP_GSMIN_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.gsmax); XmTextFieldSetString(WDG_PROP_GSMAX_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.frmin); XmTextFieldSetString(WDG_PROP_FRMIN_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.frmax); XmTextFieldSetString(WDG_PROP_FRMAX_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.xrangemin); XmTextFieldSetString(WDG_PROP_XRANGEMIN_TEXT, tmpstr);
		sprintf(tmpstr, "%ld", defaults.xrangemax); XmTextFieldSetString(WDG_PROP_XRANGEMAX_TEXT, tmpstr);
		XmTextFieldSetString(WDG_FILE_ENTRY_TEXT, "1");
	}
}

static Cursor watch = (Cursor)NULL;
void cursor_set_busy(Widget w)
{
	Display *dpy = XtDisplay(w);

	if (watch == (Cursor)NULL)
#ifdef VMS
		watch = DXmCreateCursor(w, decw$c_wait_cursor);
#else
		watch = XCreateFontCursor(dpy, XC_watch);
#endif
	XDefineCursor(dpy, XtWindow(w), watch);
	XFlush(dpy);
}


void cursor_unset_busy(Widget w)
{
	XUndefineCursor(XtDisplay(w), XtWindow(w));
}

