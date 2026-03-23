# BefEdit

A modal text editor for 2D languages, such as Befunge or ASCII art.

![demo](./assets/demo.gif)

## Installation

Run `./install.sh` to build and install. The program can then be run with `befedit`

Alternatively, the `make` command will place the `befedit` binary in `./bin/befedit`.

## How it Works

Movement is done with `h`, `j`, `k`, and `l` or arrow keys.
Unlike other editors, however, these also control a "momentum" vector.

When the editor opens, the momentum is set to `right`. If the movement key disagrees with the
momentum, it changes the momentum to that direction. If it does agree, the cursor will move.

When `i` is pressed to switch to insert mode, as you type it your cursor will move according to the
momentum. `esc` brings you back to normal mode.


## Normal Mode actions

- `.` - redo previous action (unlike other editors, movement does NOT count as an action)
- `u` - undo previous action
- `U` - redo undid action

Example of `.`:

The keystroke `itext<esc>j.` would produce

```
textt
    e
    x
    t
```

## Commands

`:` enters command mode. The following commands are currently supported:

 - `q` - quit (won't work with unsaved changes)
 - `q!` - quit without saving
 - `wq` - save and quit
 - `x` - save and quit
 - `w` - save

