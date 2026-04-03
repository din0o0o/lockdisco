# lockdisco

LED pattern sequencer for keyboard lock keys (Num Lock, Caps Lock, Scroll Lock).

Edit rhythmic patterns on a grid, set BPM and beat count, then play them back as blinking lock-key LEDs.

## Controls

| Key | Action |
|-----|--------|
| Space | Play / Stop |
| E | Enter / Exit edit mode |
| Arrows | Navigate grid / parameters |
| Space (edit) | Toggle cell |
| +/- (edit) | Adjust BPM or beat count |
| Q | Quit (restores original LED states) |

## Build

Requires GCC (MinGW/w64devkit) on Windows.

```
windres lockdisco.rc -O coff -o lockdisco.res
gcc lockdisco.c lockdisco.res -o lockdisco.exe
```
