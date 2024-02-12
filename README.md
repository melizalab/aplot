
## aplot

A time-series data analysis program for Linux

### What is it?

Aplot is a program for handling time-series data, especially:

+ sampled data recorded from sound cards, A/D converters, etc.
+ times of occurrences of neuronal action potentials (the
  spiking activity of cells in the nervous system)
+ labels that annotate times or spans of time relative to the
  above types of data.

Aplot can provide multiple views of multiple files simultaneously, scrolling and
zooming through these files very quickly, and keeping multiple plots in a window
in temporal register.

It provides the following types of plots (all with time on x-axis):

+ sonogram - frequency on y-axis, power as shades of gray/color
+ oscillogram - amplitude on y-axis
+ label - text markers that label times or spans of time
+ raster - neuronal spikes as dots relative to an event
+ PSTH - histogram of neuronal activity relative to an event
+ ISI - histogram of inter-spike intervals
+ a few other plots

It has been extensively used and developed over almost 15 years, primarily for
the study of neuronal and microphone recordings in an auditory and motor
neurophysiology setting. The interface is showing some signs of age, and it can
be difficult to compile on some platforms, but it's quite fast over a remote X11
connection and great at viewing and analyzing time-series data from different
sources.

In addition to visualizing data on-screen, aplot can produce high quality
postscript and EPS printouts and saved files of these plots. The encapsulated
postscript files can be imported into commercial drawing programs.

Aplot can also play sampled data through soundcards, mixing multiple channels
into mono or stereo channels, and marking on its plots the samples being played
in real-time.

### Installation

To install prerequisites on Debian 12:

    # apt-get install libmotif-dev libxmu-dev libxft-dev libfftw3-dev libjpeg-dev libhdf5-dev libfreetype6-dev libfontconfig1-dev scons

To compile and install:

    scons
	scons install

### Author

aplot is Copyright (c) 2002 by Amish S Dave (amish@amishdave.net). It is no longer actively maintained by Amish. This version was patched by Mike Lusignan and Dan Meliza to compile on more modern POSIX distributions and for rudimentary support of HDF5 files in ARF format (https://github.com/melizalab/arf)

### License and Disclaimer

Redistribution, use and modification of this software is permitted provided that
the above copyright and this notice and the following disclaimer are retained.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
