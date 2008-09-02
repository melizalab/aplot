#ifndef _APLOT_DEFAULTS_H_
#define _APLOT_DEFAULTS_H_

struct defaultsdata
{
	int fftsize, fftolap;
	long xmin, xmax, ymin, ymax, gsmin, gsmax, frmin, frmax, xrangemin, xrangemax;
	int xpos;
	int width;
	float zoomfactor;
	int outputwav;
	int showsliders;
	int usealsa;
	int sono_height, osc_height, label_height, rast_height, psth_height;
	int video_height;
	int isi_height, ccor_height, psip_height, spectrum_height;
	float psth_bindur;
	float psth_ymax;
	float isi_bindur;
	float isi_xrange;
	float ccor_wsize;
	float ccor_taustep;
	float ccor_tau;
	int method;
	int scale;
	int windowtype;
	int npoles;
	int colormap;

	int sono_drawxaxis, sono_drawyaxis;
	int osc_drawxaxis, osc_drawyaxis;
	int label_drawxaxis, label_drawyaxis;
	int psth_drawxaxis, psth_drawyaxis;
	int rast_drawxaxis, rast_drawyaxis, rast_autoysize, rast_dotshape;
	int isi_drawxaxis, isi_drawyaxis;
	int psip_drawxaxis, psip_drawyaxis;
	int ccor_drawxaxis, ccor_drawyaxis;
	int spectrum_drawxaxis, spectrum_drawyaxis;

	int sono_selected;
	int osc_selected;
	int label_selected;
	int psth_selected;
	int rast_selected;
	int psip_selected;
	int video_selected;
	int ccor_selected;
	int isi_selected;
};

#define TYP_SONO		1
#define TYP_OSC			2
#define TYP_PSTH		3
#define TYP_RAST		4
#define TYP_CCOR		5
#define TYP_LABEL		6
#define TYP_ISI			7
#define TYP_PSIP		8
#define TYP_VIDEO		9

#define SET_FILE 1
#define SET_ENTRY 2
#define SET_STARTSTOP 4
#define SET_FFTSIZE 8
#define SET_FFTOLAP 16
#define SET_ZOOM 32
#define SET_YRANGE 64
#define SET_GSRANGE 128
#define SET_PSTHBINDUR 256

#define DEFAULT_ZOOMFACTOR 0.10

#define DEFAULT_FFTSIZE 256
#define DEFAULT_FFTOLAP 50
#define DEFAULT_RASTDOTSHAPE 1
#define DEFAULT_PSTHBINDUR 10.0
#define DEFAULT_PSTHYMAX -1.0
#define DEFAULT_ISIBINDUR 1.0
#define DEFAULT_ISIXRANGE 250.0
#define DEFAULT_CCORWSIZE 5
#define DEFAULT_CCORTAU 100
#define DEFAULT_CCORTAUSTEP 1
#define DEFAULT_METHOD METHOD_FFT
#define DEFAULT_SCALE SCALE_DB
#define DEFAULT_WINDOWTYPE WINDOW_HAMMING
#define DEFAULT_NPOLES 40

#define DEFAULT_DOGROUPLABEL 1
#define DEFAULT_PSDOTTYPE "dot"
#define DEFAULT_OUTPUTSEQ2 0
#define DEFAULT_DEFAULTSSECTION 0

extern void init_defaults(void);
extern void load_defaults(int defsect);
extern void save_defaults(int defsect);

#endif /* _APLOT_DEFAULTS_H_ */

