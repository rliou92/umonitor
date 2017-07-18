# umonitor
Manage monitor configuration automatically

The goal of this project is to implement desktop environment independent dynamic monitor
management. Dynamic monitor management means that the positions and resolutions
of the monitors will be automatically be updated whenever monitors are
hotplugged.

Related projects:
* [autorandr](https://github.com/phillipberndt/autorandr)
* [srandrd](https://github.com/jceb/srandrd) and [screenconfig](https://github.com/jceb/screenconfig)

This program is different from existing projects because it is written entirely
in C using xcb calls and accomplishes dynamic monitor management in a single
binary. Unlike autorandr, there is no need to install rules for udev or hooks
for pmutils. Srandrd + screenconfig seems pretty promising to me, but there are
features missing such as setting the crtc xy positions.

# Installation
Run `make`. `umonitor` binary will be created in `bin`.

For Arch Linux users there is an AUR package [here](https://aur.archlinux.org/packages/umonitor-git/).

# Usage
* Setup your monitor resolutions and positions using `xrandr` or related tools (`arandr` is a good one).
* Run `umonitor --save <profile_name>`.
* Run `umonitor --listen` to begin automatically applying monitor setup.

The configuration file is stored in `~/.config/umon2.conf`. You can load a
profile manually by executing `umonitor --load <profile_name>`. Profiles can be deleted `umonitor --delete <profile_name>`.

Example scenario: You are working on a laptop. You want to save just the monitor
configuration of just the laptop screen into the profile name called 'home'. At
home you plug in an external monitor, and you want to save that configuration as
'docked'.

```
# With only the laptop screen (no external monitors)
$ umonitor --save home
Profile home saved!
home*
---------------------------------
# Plug in external monitor

# Setup your desired configuration
$ xrandr --output HDMI-1 --mode 1920x1080 --pos 1600x0
$ xrandr --output eDP1 --mode 1600x900 --pos 0x0

# Save the current configuration into a profile
$ umonitor --save docked
Profile docked saved!
home
docked*
---------------------------------
# Begin autodetecting changes in monitor
$ umonitor --listen
home
docked*
---------------------------------
# Monitor is unplugged
home*
docked
---------------------------------
```

Program help can also be viewed through `umonitor --help`.

If you would like to auto start this program, you can add the program to your .xinitrc:
```
$ cat ~/.xinitrc
#!/bin/sh
...
...
...
umonitor --listen --quiet &
exec i3 # your window manager of choice
```

# Features
I think a couple people are using this program. Give me some feedback!

What is saved and applied dynamically:
* Monitor vendor name + model number
* Crtc x and y position
* Resolution
* Primary output

Future improvements:

* Prevent duplicate profile from being saved
* Prevent program from having multiple instances
* Implement debug as compile option
* Change configuration file name to umon.conf (trivial, but a breaking change)
* More commandline options
  * Alternate configuration file location
* Updating Doxygen documentation
* Handling the case when multiple outputs are connected to same crtc?
* Bugs:
  * Tell me! Run umonitor with the `--verbose` flag to get debugging output
* Any feature requests

# Inspiration
I drew inspiration of this program from [udiskie](https://github.com/coldfix/udiskie). I enjoy using only window managers. For me, udiskie is one essential program to automount removable storage media. I would just include it in my .xinitrc and not have to worry about mounting anything manually again. I thought to myself, "Why not have the same program but for managing monitor hotplugging?"

At first, I found that the most common solutions to managing monitor hotplugging were using xrandr scripts. These scripts seemed really hacky to me, involving hardcoding of the DISPLAY environment variable. The reason why the DISPLAY environment variable needed to be hardcoded is because these scripts would be run by udev. udev runs your desired script as root and has no idea of the desired user's DISPLAY environment variable when it detects monitors being hotplugged.

The most popular program that manages monitor setups autorandr also has this problem, as it is setup to be run from a udev rule. It solves it by checking all processes not owned by root and seeing if the user of that process has a DISPLAY variable. For *all users* that do, it forks itself and changes its uid/guid to that user. Autorandr does not know which user you are, it just runs for all users!

I believed a better solution existed. By using the XCB library, I can communicate directly with the X11 server with this program running as just a single user. I do not need to rely on udev because the X11 server itself sends signals when monitors are hotplugged. Furthermore, using this program is as simple as including it in the .xinitrc, just like udiskie. For a Linux laptop user who uses a window manager only like me, I believe this software is almost a necessity for a good user experience!

# About
This is my personal project. My motivation for writing this program comes from
using i3 wm on my laptop. From writing this program I have learned a great
deal about how to interact with the X11 server, trying out OOP in C (even when
it is overkill), and hope to continue learning even more in the future. I want
to work on this project until I deem that it is "complete", fufilling its
purpose of dynamic monitor management for those who do not use a desktop
envorinment.

# Credits
I borrowed the edid parsing code from [eds](https://github.com/compnerd/eds).
