# Bibledit

The bibledit program is used editing work done during revising or
translating a Bible. To install the bibledit program, read file INSTALL.


## General programming notes

At first gtkmm-2.0 was used because of its ease of use.
But later gtk+2.x (and higher) was used, for these reasons:
- A bug in gtkmm caused the default button in a dialog not to be activated
  when Enter was pressed. Gtk+ did not have this problem.
- In gtkmm-2.4 signal_cursor_moved was suddenly removed. This was looked into
  on the gtkmm discussionlist. The reply they gave made me to lose confidence
  in gtkmm.
- In Gtk+ the Glade interface designer can be used, and that makes
  designing interfaces so much faster and simpler.


We are looking into transitioning to gtk 3.


Bibledit should be easily portable to other platforms, especially POSIX platorms.
Currently only Windows and Linux are supported. To achieve this it uses as few
libraries as possible. This is the reason that the Gnome libraries are not used.
It is also good to not use the very newest Gtk+ libraries, but wait a while until
a stable version is released and most newer distributions have them in their stock
distribution.


To measure cpu usage per function on Linux, use `valgrind --tool=callgrind bibledit-bin`.
Graphical tools to display the information that was collected:
- alleyoop
- kcachegrind
