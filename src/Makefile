# Makefile
#
#

CC = gcc
CFLAGS = -g -Wall -I/opt/local/include -I../libsono
LDFLAGS = -L/usr/local/lib -L../libsono -ldataio -lsono -lm

# libdataio and libsono
# LDFLAGS += `pkg-config --libs libjpeg`

# # hdf5 dependency
# LDFLAGS += `pkg-config --libs hdf5`

# # fftw
# LDFLAGS += `pkg-config --libs fftw3`

# # X11/Motif
# CFLAGS += `pkg-config --cflags freetype2 xmu xp ice`
# LDFLAGS += `pkg-config --libs freetype2 xmu xp ice`

INCLUDE_DIRS = -I/usr/local/include -I/usr/X11R6/include -I../../libdataio -I../../libsono
LIB_DIRS = -L/usr/local/lib -L/usr/X11R6/lib -L../../libdataio -L../../libsono -L/usr/local/lib64 -L/usr/X11R6/lib64

MOTIF_LIBS = -lXm -lXp -lXmu -lXt -lSM -lICE -lXext -lXft -lfontconfig -lXrender -lfreetype -lexpat -lX11 -lxcb -lXau -lXdmcp

SRCS = aplot.c aplot_ui.c aplot_panel.c aplot_misc.c xm_attach.c aplot_label.c aplot_rast.c aplot_psth.c aplot_isi.c aplot_ccor.c aplot_osc.c aplot_sono.c aplot_psip.c aplot_postscript.c aplot_spectrum.c aplot_video.c aplot_defaults.c aplot_x11.c XCC.c colormap.c
HDRS = aplot.h aplot_defaults.h patchlevel.h XCC.h XCCP.h

# uncomment to add support for portaudio
# HAVE_PORTAUDIO=1
# SOUND_LIBS =  -lportaudio
# SRCS += aplot_portaudio.c

OBJS = $(SRCS:.c=.o)

all: aplot

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

aplot: $(OBJS)
	$(CC) $(LDFLAGS) $(MOTIF_LIBS) -o aplot $(OBJS)

clean:
	rm -f core $(OBJS) aplot
