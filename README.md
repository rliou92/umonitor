# umonitor
Manage monitor configuration automatically

If you want something that actually works well, I would refer you to a much more mature project like <a href=https://github.com/phillipberndt/autorandr>autorandr</a>. The difference between that project and this is that my code is using xcb calls, no dependence on xrandr. 

Related projects:
      <ol>
      <li> https://github.com/jceb/srandrd / https://github.com/jceb/screenconfig
      </ol>

If you still are interested, you should be able to build the program by running <code> make </code>. Seems like the indentation is really messed up. I blame Atom text editor. It appears fine on my screen.

Usage:
       <ol>
       <li> Setup your configuration using xrandr or related tools (arandr is a good one).
       <li> Run <code> umonitor --save <profile_name> </code>
       <li> Run <code> umonitor --listen </code> to begin automatically detecting and changing monitor configuration
       </ol>
       
 The configuration file is being stored in the same directory as the program for now and is called "umon2.conf". You can load a profile manually by executing <code> umonitor --load <profile_name> </code>. This program is far from stable and has only been tested on my setup. Feedback is welcome!
 
 What's working (at least for me):
      <ol> 
      <li> Saving profile
      <li> Loading profile
      <li> Detecting which profile is currently loaded
      <li> Automatically loading a profile when a monitor is hotplugged
      </ol>
 
 What I am working on:
      <ol>
      <li> Setting and loading primary output
      <li> Parsing EDID
      <li> Refreshing after wakeup from suspend and hibernate (need help!! Is it possible to do from my one executable?)
      <li> Doxygen documentation
      <li> Configuration file follows XDG standard
      <li> Handling the case when multiple outputs are connected to same crtc (duplicate displays) (not trivial)
      <li> Systemd file for autostarting (must use systemd --user for the DISPLAY and .Xauthority environment variables)
      </ol>
      
This is my personal project. From writing this program I have learned a great deal about how to interact with the X11 server, trying out OOP in C, and I hope to continue learning even more in the future. I want to work on this project until I deem that it is "complete", fufilling its purpose of dynamic monitor management for those who do not use a desktop envorinment. 
