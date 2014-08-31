# Haiku WebKit #

This repository contains the Haiku WebKit port source code.
For more information, please visit the [project's wiki and issue tracker](http://dev.haiku-os.org/)

## Quick build instructions ##

### Cloning the repository ###

This repository is *huge* (about 5 gigabytes). If you are only interested in building
the latest version of WebKit, remember to use the --depth option to git clone.
This can be used to download only a limited part of the history and will reduce the
checkout to about 600MB. Note that WebKit uses SVN and they have the changelog inside the tree in "Changelog"
files, which you can still use as a primitive way to browse the history. You
can also use github for that, or download parts of the history later.

### Requirements ###

- You need a recent version of Haiku with the GCC 4 development tools
- The following dependencies: `CMake, GPerf, ICU, libxml, sqlite3, libxslt, Perl, Python, Ruby`
- And a fast computer!

Dependencies can be installed (for a gcc2hybrid version) via:

    $ pkgman install cmake_x86 gperf sqlite_x86_devel libxml2_x86_devel libxslt_x86_devel icu_x86_devel icu_devel perl python ruby libexecinfo_x86_devel

Or, if you build Haiku from source you can add the packages to your UserBuildConfig:

    AddHaikuImagePackages cmake_x86 gperf sqlite_x86_devel libxml2_x86_devel libxslt_devel icu_x86_devel icu_devel perl python ruby libexecinfo_x86_devel ;

Packages for other flavors of Haiku may or may not be available. Use [haikuporter](http://haikuports.org) to build them if needed.

### Building WebKit ###

#### Configuring your build for the first time ####
On a gcc2hybrid Haiku:
    $ PKG_CONFIG_LIBDIR=/boot/system/develop/lib/x86/pkgconfig \
        CC=gcc-x86 CXX=g++-x86 Tools/Scripts/build-webkit

On other versions:
    $ Tools/Scripts/build-webkit

#### Regular build, once configured ####
    $ cd WebKitBuild/Release
    $ make -j4

This will build a release version of WebKit libraries on a quad core cpu.

On a successful build, executables and libraries are generated in the WebKitBuild/Release directory.


### Advanced Build, other targets ###

The following make targets are available:

- libwtf.so - The Web Template Library
- libjavascriptcore.so -  The JavaScriptCore library
- jsc	 - The JavaScriptCore executable shell
- libwebcore.so - The WebCore library
- libwebkit.so - The WebKit library
- HaikuLauncher - A simple browsing test app
- DumpRenderTree - The tree parsing test tool

Example given, this will build the JavaScriptCore library in debug mode:

    $ make libjavascriptcore.so

In some rare cases the build system can be confused, to be sure that everything gets rebuilt from scratch,
you can remove the WebKitBuild/ directory and start over.

There are several cmake variables available to configure the build in various ways.
These can be given to build-webkit using the --cmakeargs option, or changed later on
using "cmake -Dvar=value WebKitBuild/Release".

### Speeding up the build with Ninja ###

Ninja is a replacement for Make. It is designed for use only with generated
build files (from CMake, in this case), rather than manually written ones. This
allows Ninja to remove many of Make features such as pattern-rules, complex
variable substitution, etc. As a result, Ninja is able to start building
files almost immediately, whereas Make spends several minutes scanning the
project and building the dependency tree.

To use Ninja, perform the following steps:

* First install Ninja:
    $ pkgman install ninja_x86

The build-webkit script then detects and uses Ninja automatically.

### Speeding up the build with distcc ###

You can set the compiler while calling the configure script:
    $ CC="distcc gcc-x86" CXX="distcc g++-x86" build-webkit ...

It is a good idea to set the NUMBER\_OF\_PROCESSORS environment variable as well
(this will be given to cmake through the -j option). If you don't set it, only
the local CPUs will be counted, leading to a sub-optimal distcc distribution.

distcc will look for a compiler named gcc-x86 and g++-x86. You'll need to adjust
the path on the slaves to get that pointing to the gcc4 version (the gcc4 compiler
is already visible under this name on the local machine and haiku slaves).
CMake usually tries to resolve the compiler to an absolute path on the first
time it is called, but this doesn't work when the compiler is called through
distcc.

## Testing ##

### Testing the test framework ###
    $ ruby Tools/Scripts/test-webkitruby
    $ perl Tools/Scripts/test-webkitperl
    $ python Tools/Scripts/test-webkitpy

The ruby tests pass (all 2 of them!)
The perl test almost pass: Failed 1/27 test programs. 1/482 subtests failed.
The python testsuite prints a lot of "sem\_init: No space left on device" then crashes.
But before it does, there already are some test failures.

### JSC ###
    $ perl Tools/Scripts/run-javascriptcore-tests

Add the --no-build argument if you already compiled JSC. It is built by default
for the Haiku port, so it is a good idea to always add this.

Current results:
- 9258 tests are run (some are excluded because of missing features in our Ruby port)
- 10 failures related to parsing dates and trigonometry:

    mozilla-tests.yaml/ecma_3/Date/15.9.5.6.js.mozilla
    mozilla-tests.yaml/ecma_3/Date/15.9.5.6.js.mozilla-baseline
    mozilla-tests.yaml/ecma_3/Date/15.9.5.6.js.mozilla-dfg-eager-no-cjit-validate-phases
    mozilla-tests.yaml/ecma_3/Date/15.9.5.6.js.mozilla-llint
    stress/ftl-arithcos.js.always-trigger-copy-phase
    stress/ftl-arithcos.js.default
    stress/ftl-arithcos.js.dfg-eager
    stress/ftl-arithcos.js.dfg-eager-no-cjit-validate
    stress/ftl-arithcos.js.no-cjit-validate-phases
    stress/ftl-arithcos.js.no-llint


### WebKit ###
You will have to install the Ahem font for layout tests to work properly. This
is a font with known-size glyphs that render the same on all platforms. Most of
the characters look like black squares, this is expected and not a bug!
http://www.w3.org/Style/CSS/Test/Fonts/Ahem/

$ cp LayoutTests/resources/Ahem.ttf /system/non-packaged/data/fonts/

It is also a good idea to enable automated debug reports for DumpRenderTree.
Create the file ~/config/settings/system/debug\_server/settings and add:

    exectuable_actions {
        DumpRenderTree log
    }

The crash reports will be moved from the Desktop to the test result directory
and renamed to the name of the test that triggered the crash. If you don't do
this, you have to manually click the "save report" button, and while the
testsuite waits on that, it may mark one or several tests as "timed out".

You can then run the testsuite:

    $ python Tools/Scripts/run-webkit-tests --platform=haiku --no-build \
        --no-http --no-retry-failures --clobber-old-results \
        --no-new-test-results

The options will prevent the script to try updating DumpRenderTree (it doesn't
know how to do that on Haiku, yet) and to run the HTTP tests (requires apache).
It doesn't retry failed tests, will remove previous results before starting,
and will not generate missing "expected" files in the LayoutTests directory.

A lot of tests are currently failing. The problems are either in the WebKit
code itself, or in the various parts of the test harness, none of which are
actually complete: DumpRenderTree, webkitpy, etc. Some of them are triggering
asserts in WebKit code.

### WebKit2 ###

Same as above, but:

    $ python Tools/Scripts/run-webkit-tests --platform=haiku-wk2 --no-build \
        --no-http --no-retry-failures --clobber-old-results \
        --no-new-test-results

Note that this is currently not working.

### Others ###

There are more tests, but the build-\* scripts must be working before we can run them.

## Notes ##

cmake is smart enough to detect when a variable has changed and will rebuild everything.
You can keep several generated folders with different settings if you need to switch
between them very often (eg. debug and release builds). Just invoke the build-webkit
script with different settings and different output dirs. You can then run make 
(or ninja) from each of these folders.

You can copy a WebPositive binary from your Haiku installation into the
WebKitBuild/Release folder. Launching it will then use the freshly built
libraries instead of the system ones. It is a good idea to test this because
HaikuLauncher doesn't use tabs, which sometimes expose different bugs.

This document was last updated July 30, 2014.

Authors: Maxime Simon, Alexandre Deckner, Adrien Destugues
