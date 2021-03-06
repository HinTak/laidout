LAIDOUT Version 0.096.1
=======================
http://laidout.org


Please post any complaints and other correspondence to the Laidout
general mailing list found at:
https://lists.sourceforge.net/lists/listinfo/laidout-general

Currently the only developer (and creator of Laidout) is also a client:
Tom Lechner (http://www.tomlechner.com)


WHAT IT CAN DO RIGHT NOW
------------------------
Laidout is currently in the 'mostly works on my machine' stage of development.

Laidout currently can arrange pages into various impositions,
such as a booklet, or even a dodecahedron. You can manipulate images
and gradients (linear, radial, and mesh). Also, 
you can make a type of image that can be stretched around. The very 
basics to get my cartoon books done, in other words. Many more features
are planned, like such non-essentials (to me anyway) as text! Who needs 
text when a picture is worth a thousand words? You can import
EPS or image files from other programs as a way to fake having text.

Laidout is built upon the Laxkit (http://laxkit.sourceforge.net).


COMPILING RELEASES
------------------

If you are compiling from development source, not from a release, please see
'Compiling from source' below.

You will need a few extra development libraries on your computer in order
to compile Laidout. Running ./configure does a simple check for these,
but the check is so simple, that it may miss them or otherwise get confused.
You will need the header files for Imlib2, freetype2, fontconfig, cairo, x11, xft
ssl, and cups.

If you are on a debian based system, you can probably install these with this command:

    apt-get install g++ pkg-config libpng12-dev libreadline-dev libx11-dev libxext-dev libxi-dev libxft-dev libcups2-dev libimlib2-dev libfontconfig-dev libfreetype6-dev libssl-dev xutils-dev libcairo2-dev

If you want to use the optional Opengl based unwrapper, you should also do this:

    apt-get install libgraphicsmagick++1-dev mesa-common-dev libglu1-mesa-dev libftgl-dev


Ensure that you have the Laxkit (http://laxkit.sourceforge.net)
available. If you have downloaded a release, then the Laxkit 
is included.

Here are the current "./configure" options. They are all optional:

     --help                       Show the options you can pass to ./configure
     --prefix=/where/to/install   Default /usr/local
     --gs=/path/to/gs             Where is your ghostscript executable? This is quite
                                   optional, and is used at the moment only to
                                   generate a preview image of EPS files.
     --laxkit=/laxkit/headers     Default in releases is ./laxkit/lax, since the Laxkit
                                   is currently included in release download
     --version=OtherVersionNum    You may force Laidout to compile with a different
                                   version string. This will make ~/.config/laidout/(version)
                                   and the executable becomes laidout-(version), and will
                                   save system stuff in prefix/share/laidout-(version).
     --language=/path/to/langs    The path where translation files should be installed.
                                   This will normally be prefix/share, and the files
                                   themselves get put in prefix/share/locale/*.

To compile and install Laidout, just run these three easy steps:

    ./configure
    make
    make install

Note that if your computer has, say, 8 processors, you can compile much faster
using `make -j 8` instead of plain make.

`make install` will put
laidout-(whatever version) in (usually) /usr/local/bin, and make a
symbolic link /usr/local/bin/laidout point to it.

After successfully making, the program `laidout` in the src
directory is the thing you want. It does not really depend on any other
files currently (except icons, but they are not needed to actually run),
thus it can be run from wherever it is located, but you might need to
set an alternate icon directory, since Laidout will expect icons in
the install destination.

You can do that or perhaps try out other icon sets (if you have any) by modifying
the file `~/.config/laidout/(version)/laidoutrc`. This will have been created
the first time you run Laidout. Open this file, and if the magic worked, simply
uncomment the line for `icon_dir` and set to the directory you want. For the 
uninstalled icons, this would be (this directory)/src/icons.


MAKING A DEB PACKAGE
--------------------
If you want to create a deb package of Laidout, make sure you have the fakeroot,
dpkg-dev, and debhelper packages installed, and have all the other packages listed 
from the COMPILING section above, and do this from the top laidout directory:

    dpkg-buildpackage -rfakeroot

If the magic works, you will find installable packages in the directory directly
above the Laidout directory. If it does not work, please let me know so I can fix it!

If you fail with this error:
  dpkg-shlibdeps: error: no dependency information found for /usr/lib/libGL.so.1
then you need to change: `dh_shlibdeps` in debian/rules to: `dh_shlibdeps --dpkg-shlibdeps-params=--ignore-missing-info`
Maybe something to do with non-packaged NVidia drivers?



INSTALLING DIFFERENT VERSIONS AT THE SAME TIME
----------------------------------------------

It is quite ok to install different versions of Laidout at the same time.
The ~/.config/laidout/ directory keeps all the config information in subdirectories
based on the version. The same applies for installed binaries and other
resources that get put in prefix/share/laidout. This way, you do not risk
clobbering or corrupting files from other versions. 

You can change the version number manually by specifying it when you run
./configure with somelike:

    ./configure --version=0.092-different

You should try to preserve the main version number (which is 0.092 in this
example), or it might confuse Laidout at some point.

HOWEVER, (todo) work needs to be done on "make uninstall" to not remove 
things that were installed to those directories outside of "make install". 
Right now, everything in prefix/share/laidout/version will be removed on uninstall
(but not anything in ~/.laidout). If this is causing you difficulty, please
let me know, and I'll try to finally fix this.

In any case, if you can, it's really safer to build the deb package, which 
automatically keeps track of such things, though with deb packages, you currently
can have only one at a time, without tinkering with debian/control.

Also, though you can have different versions coexist, there is not currently an
automatic way to convert resources from older laidout versions to newer ones.
Again, if this is a problem, let me know!


COMPILING FROM SOURCE
---------------------
For the Laidout development version, you will need the Laxkit development 
version from the same date, give or take 10 minutes or so. Be advised that
the dev version may be rather buggy compared to "stable" versions!

Here is a fast and easy way to get Laidout up and running from development source:

1. Grab Laidout source from github (make sure the git package is installed):
        git clone https://github.com/Laidout/laidout.git

2. Grab the Laxkit submodule:
   Enter the laidout directory and execute this to initialize the laxkit submodule:

        cd laidout
        git submodule update --init

3. Compile the goods, by doing this:

        cd laxkit
        ./configure
        make depends
        cd ..
        ./configure
        make depends
        make

    Note that the Makefiles in git are for the development source, which
    also tries to do `make` of the Laxkit, since Laxkit and Laidout are 
    being developed basically side by side. Just remove those parts of the Makefiles
    if you want to compile only Laidout.

    Also note that if your computer has, say, 8 processors, you can compile much faster
    using `make -j 8` instead of plain make.

4.  Icons.
    Steps 1-3 have (ideally) compiled everything. You can run Laidout (at src/laidout)
    without icons and also without installing it anywhere.

    When downloading development source code, the icons have not yet been generated. 
    Making the icons requires Inkscape (http://www.inkscape.org) to generate icons 
    from src/icons/icons.svg, and in laxkit/lax/icons/icons.svg. A python script there 
    calls Inkscape. cd to src/icons and run "make". Do the same for laxkit/lax/icons/.

    The laxkit icons need to be put in the laidout icon area. do this with:

        cp laxkit/lax/icons/*png src/icons

    All this in one step:

        cd laxkit/lax/icons; make; cp *png ../../../src/icons; cd ../../../src/icons; make

    Some day, the icons will themselves be stored in a Laidout file, and Laidout itself
    will generate the icons (maybe!). The current system is a bit cumbersome.

    You can use src/icons/makeimages.py to regenerate all icons, or single icons if you
    like. The master icon file is src/icons.svg. Each top level object in that file
    with ids that start with a capital letter will have an icon generated of the same
    name by running makeimages.py ThatName.

5.  Running Laidout might spit out copious amounts of debugging info to stderr
    if you run from a terminal. If this is the case, you can turn this 
    off with `make hidegarbage` before doing `make` (the same goes for the 
    Laxkit). Be advised that this requires Perl to be installed. Or, you 
    can just run Laidout like this:

        ./laidout 2> /dev/null


SOURCE CODE DOCUMENTATION and CONTRIBUTING
------------------------------------------

There is a lot of source code documentation available via doxygen, just type:

    make docs

or if you want all documentation for the Laxkit and Laidout in one place, then
make sure you have a link to the Laxkit source in the top Laidout directory,
and do:

    make alldocs
 
This will dump it all out to docs/html. In particular, you will have a 
rapidly expanding todo list. If you want to contribute, that is a good
place to start. That and the Laxkit todo list.

A down and dirty way to become aquainted with the source code is to 
download the Laxkit to the top laidout directory (if it is not there
already), and change the following lines in the Doxyfile-with-laxkit
to say YES rather than NO:

    SOURCE_BROWSER         = YES
    EXTRACT_ALL            = YES

Then from the docs directory, run `doxygen Doxyfile-with-laxkit`
(or do make alldocs from top directory), which will generate a clickable source
tree accessible from your favorite web browser.

By the way, all the code assumes tabs are 4 characters wide.
(in vim, :set tabstop=4)

Also check out the dev page on the Laidout website:
http://www.laidout.org/dev.html


Compiling on Mac OS X
---------------------
For the adventurous, you can help make Laidout work on OS X.
Straight from downloading, it will not compile all the way on OS X.
You'll still need x11, xft, Imlib2, freetype2, fontconfig, ssl, and cups.
I'm just guessing, since I don't have access to an OS X or other BSD
based machine, but it shouldn't be too hard to make Laidout work on them.
There are a few places where I use GNU-only functions, like getline(),
which shouldn't be very difficult to replace or define similar functions.


Compiling on Windows
--------------------
Sorry, you're out of luck at the moment. If this is important to you,
and you don't want to figure it out yourself, feel free to subsidize
a windows port with copious amounts of money.


