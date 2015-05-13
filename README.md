Final Burn Alpha for Raspberry Pi
---------------------------------

Final Burn Alpha is a multi-system emulator. This particular port is
based on the SDL version of the emulator with native GLES-based
rendering for the Raspberry Pi.

Source is based on
[FBA for OpenDingux](https://github.com/dmitrysmagin/fba-sdl), sans the
OpenDingux-specific stuff.

Other Features
------------------------------

This version adds a handful of other features, including:

* Configurable joystick inputs
* Kiosk mode (shuts down the emulator after a period of inactivity)
* NVRAM saves

To enable NVRAM saves, create an `nvram` subdirectory in the same directory
as the executable.

I've also made some minor changes to make the SDL version compilable (though
not really usable) on OS X.

Compiling
---------

To compile on a Raspberry Pi, install SDL and udev libraries:

`sudo apt-get install libsdl-dev`
`sudo apt-get install libudev-dev`

Run `make pi` to start the build - it will take a while.

To compile on a Mac, run `make sdl`. You will need the SDL libraries:
you can get them via [Homebrew](http://brew.sh/) by running `brew install sdl`.

Quitting
--------

To exit the emulator, press `F12`.

Supported Games
---------------

See [gamelist.txt](gamelist.txt).

License
-------

All auxiliary code by me (Akop Karapetyan) is released under the [Apache
license](http://www.apache.org/licenses/LICENSE-2.0) and identified as
such in the individual file headers.

Core components of FBA are licensed under a more restrictive license -
see [license.txt](src/license.txt) for details.

Credits
-------------

Parts of code are from [fba-sdl](https://github.com/dmitrysmagin/fba-sdl)
by Dmitry Smagin.

FinalBurn Alpha is written by the
[FB Alpha Team](http://www.barryharris.me.uk/).
