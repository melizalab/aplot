# Makefile 
#
#
OPT_CFLAGS = -O2 -g2 -Wall
PTHREAD_LIBS = -lpthread

FFTW_DEPS = -DHAVE_FFTW3
FFTW_LIBS = -lfftw3

SONO_DEPS = -I/usr/local/include
SONO_LIBS = -L/usr/local/lib -lsono

DATAIO_DEPS = -I/usr/local/include
DATAIO_LIBS = -ljpeg -L/usr/local/lib -ldataio # -lmad

MOTIF_DEPS = -I/usr/X11R6/include
MOTIF_LIBS = -L/usr/X11R6/lib -lXm -lXp -lXmu -lXt -lSM -lICE -lXext -lX11

#ALSA_DEPS = -DHAVE_ALSA
#ALSA_LIBS = -lasound
#ALSA_FILES = play_alsa.c

CFLAGS	= ${OPT_CFLAGS} ${FFTW_DEPS} ${SONO_DEPS} ${DATAIO_DEPS} ${MOTIF_DEPS} ${ALSA_DEPS}
ALL_LIBS = ${PTHREAD_LIBS} ${FFTW_LIBS} ${SONO_LIBS} ${DATAIO_LIBS} ${MOTIF_LIBS} ${ALSA_LIBS} -lm

SRCS = aplot.c aplot_ui.c aplot_panel.c aplot_misc.c xm_attach.c\
       aplot_label.c aplot_rast.c aplot_psth.c aplot_isi.c aplot_ccor.c\
	   aplot_osc.c aplot_sono.c aplot_psip.c aplot_postscript.c\
	   aplot_spectrum.c aplot_video.c \
	   aplot_defaults.c play_oss.c ${ALSA_FILES} \
	   aplot_x11.c XCC.c colormap.c
HDRS = aplot.h aplot_defaults.h patchlevel.h XCC.h XCCP.h
OBJS = $(SRCS:.c=.o)

TVER := $(shell grep BUILD version.h | cut -d' ' -f3)
BUILDVER := $(shell expr 1 + ${TVER})

all: aplot

aplot: ${OBJS} 
	$(CC) ${OPT_CFLAGS} -o aplot $(OBJS) ${ALL_LIBS}

aplot-static: ${OBJS}
	$(CC) -static ${OPT_CFLAGS} -o aplot $(OBJS) ${ALL_LIBS}

clean:
	rm -f core $(OBJS) aplot

spotless: clean
	rm -f .depend

dep depend:
	$(CPP) -M $(CFLAGS) $(SRCS) > .depend

install: aplot
	make -C ../ install

release:
	-rm -f version.h
	-echo "#define APLOT_BUILD "$(BUILDVER) > version.h
	-echo "#define APLOT_VERSION "`date +%Y%m%d` >> version.h

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif

