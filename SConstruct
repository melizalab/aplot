import os

if hasattr(os, "uname"):
    system = os.uname()[0]
else:
    system = "Windows"

# install location
AddOption(
    "--prefix",
    dest="prefix",
    type="string",
    nargs=1,
    action="store",
    metavar="DIR",
    help="installation prefix",
)
# debug flags for compliation
debug = ARGUMENTS.get("debug", 0)

if not GetOption("prefix") == None:
    install_prefix = GetOption("prefix")
else:
    install_prefix = "/usr/local/"

Help(
    """
Type: 'scons'  to build aplot
      'scons install' to install program under %s
      (use --prefix  to change library installation location)

Options:
      debug=1      to enable debug compliation
"""
    % install_prefix
)

env = Environment(
    ENV=os.environ,
    CCFLAGS=["-Wall"],
    LIBS=["m"],
    PREFIX=install_prefix,
    tools=["default"],
)

if "CC" in os.environ:
    env.Replace(CC=os.environ["CC"])

env.Append(CPPPATH=["#libsono", "#libdataio"])

env.Append(LIBS=["Xm"])
env.ParseConfig(
    "pkg-config xmu xt sm ice xext xft fontconfig xrender freetype2 xcb xau xdmcp --cflags --libs"
)
env.ParseConfig("pkg-config libjpeg --cflags --libs")
if system == "Darwin":
    env.Append(CPPPATH=["/opt/local/include"], LIBPATH=["/opt/local/lib"])
else:
    env.ParseConfig("pkg-config hdf5 --cflags --libs")
    env.ParseConfig("pkg-config fftw3 --cflags --libs")

if int(debug):
    env.Append(CCFLAGS=["-g2"])
else:
    env.Append(CCFLAGS=["-O2"])

srcs = (
    env.Glob("libsono/*.c")
    + env.Glob("libdataio/*.c")
    + [
        "src/aplot.c",
        "src/aplot_ui.c",
        "src/aplot_panel.c",
        "src/aplot_misc.c",
        "src/xm_attach.c",
        "src/aplot_label.c",
        "src/aplot_rast.c",
        "src/aplot_psth.c",
        "src/aplot_isi.c",
        "src/aplot_ccor.c",
        "src/aplot_osc.c",
        "src/aplot_sono.c",
        "src/aplot_psip.c",
        "src/aplot_postscript.c",
        "src/aplot_spectrum.c",
        "src/aplot_video.c",
        "src/aplot_defaults.c",
        "src/aplot_x11.c",
        "src/XCC.c",
        "src/colormap.c",
    ]
)

prog = env.Program("aplot", srcs)
env.Alias("install", env.Install(os.path.join(env["PREFIX"], "bin"), prog))
