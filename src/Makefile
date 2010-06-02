# Makefile 
#
#

CC=/usr/bin/gcc
ARCH_CFLAGS = -O2 -g2
PTHREAD_LIBS = -lpthread

FFTW_DEPS = -DHAVE_FFTW3
FFTW_LIBS = -lfftw3

INCLUDE_DIRS = -I/usr/local/include -I/usr/X11R6/include -I/sw/include
LIB_DIRS = -L/usr/local/lib -L/usr/X11R6/lib -L/sw/lib -L/usr/local/lib64 -L/usr/X11R6/lib64

HDF5_LIBS = -lhdf5
SONO_LIBS =  -lsono
DATAIO_LIBS = -ljpeg -ldataio
MOTIF_LIBS = -lXm -lXp -lXmu -lXt -lSM -lICE -lXext -lX11
SOUND_LIBS =  -lportaudio

CFLAGS	= ${ARCH_CFLAGS} ${INCLUDE_DIRS}
ALL_LIBS = ${LIB_DIRS} ${PTHREAD_LIBS} ${SONO_LIBS} ${FFTW_LIBS}  ${DATAIO_LIBS} ${MOTIF_LIBS} ${SOUND_LIBS} ${HDF5_LIBS} -lm

SRCS = aplot.c aplot_ui.c aplot_panel.c aplot_misc.c xm_attach.c\
       aplot_label.c aplot_rast.c aplot_psth.c aplot_isi.c aplot_ccor.c\
	   aplot_osc.c aplot_sono.c aplot_psip.c aplot_postscript.c\
	   aplot_spectrum.c aplot_video.c \
	   aplot_defaults.c play_portaudio.c \
	   aplot_x11.c XCC.c colormap.c
HDRS = aplot.h aplot_defaults.h patchlevel.h XCC.h XCCP.h
OBJS = $(SRCS:.c=.o)

TVER := $(shell grep BUILD version.h | cut -d' ' -f3)
BUILDVER := $(shell expr 1 + ${TVER})

all: aplot

aplot: ${OBJS} 
	$(CC) ${CFLAGS} -o aplot $(OBJS) ${ALL_LIBS}

aplot-static: ${OBJS}
	$(CC) -static ${CFLAGS} -o aplot $(OBJS) ${ALL_LIBS}

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

