import os

if hasattr(os,'uname'):
    system = os.uname()[0]
else:
    system = 'Windows'

# install location
AddOption('--prefix',
          dest='prefix',
          type='string',
          nargs=1,
          action='store',
          metavar='DIR',
          help='installation prefix')
# debug flags for compliation
debug = ARGUMENTS.get('debug', 0)

if not GetOption('prefix')==None:
    install_prefix = GetOption('prefix')
else:
    install_prefix = '/usr/local/'

Help("""
Type: 'scons'  to build aplot
      'scons install' to install program under %s
      (use --prefix  to change library installation location)

Options:
      debug=1      to enable debug compliation
""" % install_prefix)

env = Environment(ENV = os.environ,
                  CCFLAGS=['-Wall'],
                  LIBS=['m'],
                  PREFIX=install_prefix,
                  tools=['default'])

if "CC" in os.environ:
    env.Replace(CC=os.environ['CC'])

if system=='Darwin':
    env.Append(CPPPATH=['/opt/local/include'],
               LIBPATH=['/opt/local/lib'])
else:
    env.ParseConfig("pkg-config fftw3 --cflags --libs")

if int(debug):
    env.Append(CCFLAGS=['-g2'])
else:
    env.Append(CCFLAGS=['-O2'])

sono = SConscript("libsono/SConscript", exports="env")
SConscript("src/SConscript", exports='env sono')
