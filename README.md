tilel
=====

Scriptable XCB-based tiler for EWMH-compliant window managers.

Usage
-----

This program merely stores the windows managed by the window manager in an order
only changeable by user commands.

Commands are read from a named pipe, `/tmp/tilel.fifo` -- one might want to set
keybindings with e.g. `xbindkeys`.

Then, it looks for an executable in the user's home directory, `~/.tilel`, and
passes three parameters to it, screen width, screen height and number of
windows.

For each window in order, the executable shall output its dimensions -- x, y,
width and height, separated each by a single non-digit character. An example
script is included.
