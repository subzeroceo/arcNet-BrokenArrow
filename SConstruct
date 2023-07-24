# -*- mode: python -*-
import sys, os, time, commands, re, pickle, StringIO, popen2, commands, pdb, zipfile, string
import SCons

sys.path.append( 'sys/scons' )
import scons_utils

conf_filename='site.conf'
# choose configuration variables which should be saved between runs
# ( we handle all those as strings )
serialized=['CC', 'CXX', 'JOBS', 'BUILD', 'GL_HARDLINK',
	'DEBUG_MEMORY', 'LIBC_MALLOC', 'ID_MCHECK', 'ALSA',
	'TARGET_CORE', 'TARGET_MONO, 'SDK', 'NOCURL',
	'BUILD_ROOT', 'SILENT' ]

g_sdk = not os.path.exists( 'sys/scons/SConscript.core' )


help_string = """
Usage: scons [OPTIONS] [TARGET] [CONFIG]

[OPTIONS] and [TARGET] are covered in command line options, use scons -H

[CONFIG]: KEY="VALUE" [...]
a number of configuration options saved between runs in the """ + conf_filename + """ file
erase """ + conf_filename + """ to start with default settings again

CC (default gcc)
CXX (default g++ )
	Specify C and C++ compilers (defaults gcc and g++ )
	ex: CC="gcc-3.3"
	You can use ccache and distcc, for instance:
	CC="ccache distcc gcc" CXX="ccache distcc g++"

JOBS (default 1 )
	Parallel build

BUILD (default debug)
	Use debug-all/debug/release to select build settings
	ex: BUILD="release"
	debug-all: no optimisations, debugging symbols
	debug: -O -g
	release: all optimisations, including CPU target etc.

BUILD_ROOT (default 'build')
	change the build root directory

NOCONF (default 0, not saved)
	ignore site configuration and use defaults + command line only

SILENT ( default 0, saved )
	hide the compiler output, unless error
"""

if ( not g_sdk ):
	help_string += """
TARGET_CORE (default 1 )
	Build the core

TARGET_MONO (default 0 )
	Build a monolithic binary

TARGET_ENGINE\

GL_HARDLINK (default 0 )
	Instead of dynamically loading the OpenGL libraries, use implicit dependencies
	NOTE: no GL logging capability and no r_glDriver with GL_HARDLINK 1

DEBUG_MEMORY (default 0 )
	Enables memory logging to file

LIBC_MALLOC (default 1 )
	Toggle aRcHeap memory / libc malloc usage
	When libc malloc is on, memory size statistics are wrong ( no _msize )

ID_MCHECK (default 2)
	Perform heap consistency checking
	0: on in Debug / off in Release
	1 forces on, 2 forces off
	note that Doom has it's own block allocator/checking
	this should not be considered a replacement, but an additional tool

ALSA (default 1 )
	enable ALSA sound backend support

SETUP (default 0, not saved)
    build a setup. implies release build

SDK

NOCURL (default 0 )
	set to 1 to disable usage of libcurl and http/ftp downloads feature
"""

Help( help_string )

EnsureSConsVersion( 0, 96 )

cpu = commands.getoutput('uname -m')
exp = re.compile('.*i?86.*')
if exp.match(cpu):
	cpu = 'x86'
else:
	cpu = commands.getoutput('uname -p')
	if ( cpu == 'powerpc' ):
		cpu = 'ppc'
	else:
		cpu = 'cpu'
g_os = 'Linux'

CC = 'gcc'
CXX = 'g++'
JOBS = '1'
BUILD = 'debug'
TARGET_CORE = '1'
TARGET_MONO = '0'
GL_HARDLINK = '0'
DEBUG_MEMORY = '0'
LIBC_MALLOC = '1'
ID_MCHECK = '2'
BUILD_ROOT = 'build'
ALSA = '1'
SETUP = '0'
NOCONF = '0'
NOCURL = '0'
SDK = ''
SILENT = '0'


if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	site_dict = {}
	if (os.path.exists(conf_filename) ):
		site_file = open(conf_filename, 'r')
		p = pickle.Unpickler(site_file)
		site_dict = p.load()
		print 'Loading Build Configuration From: ' + conf_filename + ':'
		for k, v in site_dict.items():
			exec_cmd = k + '=\'' + v + '\''
			print '  ' + exec_cmd
			exec(exec_cmd)
else:
	print 'Site Settings Ignored'

for k in ARGUMENTS.keys():
	exec_cmd = k + '=\'' + ARGUMENTS[k] + '\''
	print 'Command line: ' + exec_cmd
	exec( exec_cmd )


if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	for k in serialized:
		exec_cmd = 'site_dict[\'' + k + '\'] = ' + k
		exec(exec_cmd)

	site_file = open(conf_filename, 'w')
	p = pickle.Pickler(site_file)
	p.dump(site_dict)
	site_file.close()

if ( SETUP != '0' ):
	BUILD		= 'release'

#if ( g_sdk or SDK != '0' ):
	TARGET_CORE = '0'
	TARGET_MONO = '0'

g_build = BUILD_ROOT + '/' + BUILD

SConsignFile( 'scons.signatures' )

if ( GL_HARDLINK != '0' ):
	g_build += '-hardlink'

if ( DEBUG_MEMORY != '0' ):
	g_build += '-debugmem'

if ( LIBC_MALLOC != '1' ):
	g_build += '-nolibcmalloc'

SetOption('num_jobs', JOBS)

LINK = CXX

# common flags
# BASE + CORE + OPT for engine
# _noopt versions of the environements are built without the OPT

BASECPPFLAGS = [ ]
CORECPPPATH = [ ]
CORELIBPATH = [ ]
CORECPPFLAGS = [ ]
BASELINKFLAGS = [ ]
CORELINKFLAGS = [ ]

# for release build, further optimisations that may not work on all files
OPTCPPFLAGS = [ ]

BASECPPFLAGS.append( BASEFLAGS )
BASECPPFLAGS.append( '-pipe' )
# warn all
BASECPPFLAGS.append( '-Wall' )
BASECPPFLAGS.append( '-Wno-unknown-pragmas' )
# this define is necessary to make sure threading support is enabled in X
CORECPPFLAGS.append( '-DXTHREADS' )
# don't wrap gcc messages
BASECPPFLAGS.append( '-fmessage-length=0' )
# gcc 4.0
BASECPPFLAGS.append( '-fpermissive' )

if ( g_os == 'Linux' ):
	# gcc 4.x option only - only export what we mean to from the game SO
	BASECPPFLAGS.append( '-fvisibility=hidden' )
	# get the 64 bits machine on the distcc array to produce 32 bit binaries :)
	BASECPPFLAGS.append( '-m32' )
	BASELINKFLAGS.append( '-m32' )
if ( BUILD == 'debug-all' ):
	OPTCPPFLAGS = [ '-g', '-D_DEBUG' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'debug' ):
	OPTCPPFLAGS = [ '-g', '-O1', '-D_DEBUG' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'release' ):
	# -fomit-frame-pointer: "-O also turns on -fomit-frame-pointer on machines where doing so does not interfere with debugging."
	#   on x86 have to set it explicitely
	# -finline-functions: implicit at -O3
	# -fschedule-insns2: implicit at -O2
	# no-unsafe-math-optimizations: that should be on by default really. hit some wonko bugs in physics code because of that
	OPTCPPFLAGS = [ '-O3', '-march=pentium3', '-Winline', '-ffast-math', '-fno-unsafe-math-optimizations', '-fomit-frame-pointer' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '2'
else:
	print 'Unknown build configuration ' + BUILD
	sys.exit(0 )

if ( GL_HARDLINK != '0' ):
	CORECPPFLAGS.append( '-DID_GL_HARDLINK' )

if ( DEBUG_MEMORY != '0' ):
	BASECPPFLAGS += [ '-DID_DEBUG_MEMORY', '-DARC_REDIRECT_NEWDELETE' ]

if ( LIBC_MALLOC != '1' ):
	BASECPPFLAGS.append( '-DUSE_LIBC_MALLOC=0' )

if ( ID_MCHECK == '1' ):
	BASECPPFLAGS.append( '-DID_MCHECK' )

# create the build environements
g_base_env = Environment( ENV = os.environ, CC = CC, CXX = CXX, LINK = LINK, CPPFLAGS = BASECPPFLAGS, LINKFLAGS = BASELINKFLAGS, CPPPATH = CORECPPPATH, LIBPATH = CORELIBPATH )
scons_utils.SetupUtils( g_base_env )

g_env = g_base_env.Clone()

g_env['CPPFLAGS'] += OPTCPPFLAGS
g_env['CPPFLAGS'] += CORECPPFLAGS
g_env['LINKFLAGS'] += CORELINKFLAGS

g_env_noopt = g_base_env.Clone()
g_env_noopt['CPPFLAGS'] += CORECPPFLAGS

g_game_env = g_base_env.Clone()
g_game_env['CPPFLAGS'] += OPTCPPFLAGS
g_game_env['CPPFLAGS'] += ENGINECPPFLAGS

# maintain this dangerous optimization off at all times
g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )
g_env_noopt.Append( CPPFLAGS = '-fno-strict-aliasing' )
g_game_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

if ( int( JOBS ) > 1 ):
	print 'Using buffered process output'
	silent = False
	if ( SILENT == '1' ):
		silent = True
	scons_utils.SetupBufferedOutput( g_env, silent )
	scons_utils.SetupBufferedOutput( g_game_env, silent )

# mark the globals

# 0 for monolithic build
localEngineDLL = 1
# carry around rather than using .a, avoids binutils bugs
idLibObjects = []
engineObjects = []
# curl usage. there is a global toggle flag
local_curl = 0
curl_lib = []
# if arcLibrary should produce PIC objects ( depending on core or game inclusion )
localLibPic = 0
# switch between base game build and d3xp game build

GLOBALS = 'g_env g_env_noopt g_os ID_MCHECK ALSA idLibObjects localLibPic curl_lib local_curl OPTCPPFLAGS'

Export( 'GLOBALS ' + GLOBALS )

engine = None
gameEngine = None
engineMono = None

# build curl if needed
if ( NOCURL == '0' and ( TARGET_CORE == '1' or TARGET_MONO == '1' ) ):
	# 1: debug, 2: release
	if ( BUILD == 'release' ):
		local_curl = 2
	else:
		local_curl = 1
	Export( 'GLOBALS ' + GLOBALS )
	curl_lib = SConscript( 'sys/scons/SConscript.curl' )

if ( TARGET_CORE == '1' ):
	localLibPic = 0
		Export( 'GLOBALS ' + GLOBALS )
		VariantDir( g_build + '/core/glimp', '.', duplicate = 1 )
		SConscript( g_build + '/core/glimp/sys/scons/SConscript.gl' )
		VariantDir( g_build + '/core', '.', duplicate = 0 )
		idLibObjects = SConscript( g_build + '/core/sys/scons/SConscript.arcLibrary' )
		Export( 'GLOBALS ' + GLOBALS ) # update idLibObjects
		engine = SConscript( g_build + '/core/sys/scons/SConscript.core' )

		InstallAs( '#engine.' + cpu, engine )
if ( TARGET_MONO == '1' ):
	localEngineDLL = 0
	localLibPic = 0
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/mono/glimp', '.', duplicate = 1 )
	SConscript( g_build + '/mono/glimp/sys/scons/SConscript.gl' )
	VariantDir( g_build + '/mono', '.', duplicate = 0 )
	idLibObjects = SConscript( g_build + '/mono/sys/scons/SConscript.arcLibrary' )
	engineObjects = SConscript( g_build + '/mono/sys/scons/SConscript.game' )
	Export( 'GLOBALS ' + GLOBALS )
	engineMono = SConscript( g_build + '/mono/sys/scons/SConscript.core' )
	InstallAs( '#engine-mon.' + cpu, engineMono )

if ( SETUP != '0' ):
	brandelf = Program( 'brandelf', 'sys/linux/setup/brandelf.c' )
	if ( TARGET_CORE == '1' and TARGET_ENGINE == '1' ):
		setup = Command( 'setup', [ brandelf, engine, gameEngine ], Action( g_env.BuildSetup ) )
if ( SDK != '0' ):
	setup_sdk = Command( 'sdk', [ ], Action( g_env.BuildSDK ) )
	g_env.Depends( setup_sdk, [ engine ] )

# end targets ------------------------------------
