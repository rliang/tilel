tilel
=====

The windows managed by the window manager are stored in an order only changeable
by user commands.

Commands are read from a named pipe, `/tmp/tilel.fifo` -- one might want to set
keybindings with e.g. `xbindkeys` to write into it.

Then, it looks for an executable in the user's home directory, `~/.tilel`, and
passes three parameters to it, screen width, screen height and number of
windows.

For each window in order, the executable shall output its dimensions -- x, y,
width and height, separated each by a single non-digit character. An example
script is included.

Commands
--------

`F[index]`: Focus window at index `index`.

`f[count]`: Focus window at `count` indexes relative to the currently focused
window.

`M[index]`: Move focused window to index `index`.

`m[index]`: Move focused window `count` indexes relative to the currently
focused window.

Examples
--------

Focus the window at index 0: `echo F0 > /tmp/tilel.fifo`

Focus the previous window: `echo f-1 > /tmp/tilel.fifo`

Move the focused window to index 3: `echo M3 > /tmp/tilel.fifo`

Move the focused window forward two indexes: `echo m2 > /tmp/tilel.fifo`
