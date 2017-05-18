# umonitor
Manage monitor configuration automatically

The goal of this project is to implement desktop independent dynamic monitor
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

A systemd unit has been included for autostarting the application and can be
enabled with `systemd --user enable umonitor`.

In order to automatically apply updates after suspending, I have created a
systemd unit called `udev_trigger` as a hack to trigger udev to send the screen
change signal. This systemd unit should be enabled with the system systemd:
`systemd enable --now udev_trigger`.

# Usage
* Setup your configuration using `xrandr` or related tools (`arandr` is a good one).
* Run `umonitor --save <profile_name>`.
* Run `umonitor --listen` to begin automatically applying monitor configuration.


 The configuration file is stored in `~/.config/umon2.conf`. You can load a
 profile manually by executing `umonitor --load <profile_name>`.

# Features
This program has only been tested on my setup. I need testers and feedback!


What is saved and applied dynamically:
* Monitor vendor name + model number
* Crtc x and y position
* Resolution
* Primary output
* Will not load duplicate crtcs

Future improvements:

* Expanding Doxygen documentation
* More commandline options
  * Alternate configuration file location
* Clean up output
  * Custom debug function
* Refresh configuration file when it is changed while listening to events
* Handling the case when multiple outputs are connected to same crtc
* Inspect why udev is not being triggered after wakeup from suspend and
hibernate (udev bug?)
* I could just use `xrandr` to load the changes, might be a lot easier that way

# Conclusion
This is my personal project. From writing this program I have learned a great
deal about how to interact with the X11 server, trying out OOP in C (even when
it is overkill), and hope to continue learning even more in the future. I want
to work on this project until I deem that it is "complete", fufilling its
purpose of dynamic monitor management for those who do not use a desktop
envorinment.