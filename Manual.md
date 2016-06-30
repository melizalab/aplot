 This page is for notes about using aplot, a data visualization program written by Amish Dave. It collects documentation formerly scattered over many lands.

### Viewing data

Aplot provides a number of representations for viewing acoustic and electrophysiological data (more generally, sampled and point process time series). It is especially useful when there are multiple data channels or sources for the same time interval; for example, viewing the sonogram of a stimulus along with a raster plot of a neural response.

Viewing data in aplot is a two-stage operation. First, data files must be added to the `File` list on the left of the main window. Second, data files are selected to be plotted and added to panels show in the `Panel` list on the right side of the main window. Data files can be added to the `File` list either as arguments when starting the program or by clicking `Add` below the `File` list. For example, `aplot *.wav` will load all the `.wav` files in the current directory into the `File` panel.

The supported plot types are as follows:

- `Sono` shows the power spectrogram (or sonogram) of a discrete sampled time series
- `Osc` shows the waveform of a sampled time series
- `Label` shows a labeled point process as a series of labeled vertical lines
- `PSTH` shows a histogram of point process event times
- `Raster` shows a raster plot of a point process, in which each repetition of a stimulus is shown in a separate row
- `ISI` shows an inter-event-interval histogram for a point process.

To view a file, select it in the `File` list, select the desired plot type in the plot list, and click the `Add` button to the left of the `Panel` list. You may select more than one plot type at a time, but this is not recommended. Each plot lives in a panel (or window). If no panels are open when you click `Add`, a new panel will be created. If you select an existing panel and click `Add`, the plot will be added to it. Plots can be removed from panels. Note that you can select one or more plots in the `Panel` list, which will influence the effects of subsequent actions. All plots in a panel are generally assumed to represent the same interval of time.

### Basic movement and navigation

Regions of a panel can be selected by clicking and dragging with the middle mouse button. Only currently active groups are affected. To select in all the groups in a panel, first select the panel in the main aplot window. Once a region is selected, the following keys can be used to zoom and pan:

* down arrow key: zoom in to selected region (or to a region 10% the size of the current region if no selection)
* up arrow key: zoom out to previous zoom level
* left/right arrow keys: shift view panel to earlier or later times (if zoomed in)
* shift-left/shift-right: shift view panel by a smaller amount

### Keyboard Shortcuts

These keys and key combinations have the following effects when pressed while the mouse pointer is over a group

* Shift-p : play data in current panel. Restricted to a region if defined. Soundcard support is needed.
* Shift-s : save data in current panel into a file (e.g. wave for sono or osc plots). Restricts to selected region if defined.
* Ctrl-p : print current panel to lpr
* '>' : move to next entry (in pcm_seq2 files)
* '<' : move to previous entry

The following keys have special effects in sono plots:

* = : decrease maximum of color map, darkening features with strong power
* + (plus): increase max of color map, lightening features
* - (minus): decrease min of color map, darkening the background
* _ : increase min of color map, lightening the background

When the mouse is over a label plot, striking any letter key will cause a label to be produced for that time point. Similarly, if a region is selected, labels for the start and stop of that interval will be created.

### Label Files

To open a label file, click the "label" option in the "Plot" column of aplot window. You have to already have another file (song, EEG, etc.) opened. If this a new label file, click "add" on the right side of the aplot window...the label file will have the same name as the file already opened but with a .lbl extension. If you have already already created a label file, open it using the "add" option on the left side of the aplot window. There should now be a blank (or previously labeled) panel associated with the song, EEG, etc. file.

You can create 2 types of labels:

* Instantaneous: This marks an event that happens at single point in time with no duration. To create this label, slide the mouse on the label panel to where you want to mark the event and press any key. A vertical line at that time point will appear with whatever key label you gave it.

* Extended: This marks an event that has a duration (e.g., song syllable, sleep state, movement artifact, etc.). If you are using a 3-button mouse, use the center button and click on the label panel where the event starts or stops. Keep the center button held, slide the mouse to the other endpoint of the event, and release. Two vertical lines should appear. Press any key to label the event, and the two lines will be connected and labeled. If you don't have a 3-button mouse, you can emulate a 3-button mouse in X11. Open the X11 preferences, click on the "input" tab, and click on the "emulate 3-button mouse" option. I've only tried this on a mac, but the extended event can be marked by left-clicking and holding the "control" and "option" keys.

The .lbl file is a text file. It begins with 7 lines of nonsense that is required. After that, the left column represents the "time" point of the label (how the time relates to seconds depends on the sampling rate), followed by "121", and then the label. "Instantaneous" labels simply are the key you pressed. Extended events are marked by "-0" and "-1", where -0 is the onset time and -1 is the offset time (e.g., syllable "a" will be marked as "a-0" and "a-1".
Defaults

### Configuration Files

Aplot can save configuration data to defaults files, which are located in ~/.aplot/ (you need to make this directory, or no configuration data will be saved). The default file to be used in a given aplot session can be set by giving aplot the argument "-def <number>". When exiting aplot, hold down the shift key while clicking exit to save the parameters.

### Other random notes

Hold down the shift key when clicking print to print to an eps file (instead of lpr)
