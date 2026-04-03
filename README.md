[![Download dInk](https://img.shields.io/github/v/release/din0o0o/lockdisco?label=Download&color=ffeb3b&labelColor=2d2d2d&style=for-the-badge)](https://github.com/din0o0o/lockdisco/releases/latest)

> # $\color{#ffeb3b}\{\textbf{dInk}}$
</br>

LED pattern sequencer for Num, Caps and Scroll Lock.

Edit patterns, set BPM and length, then play them back as blinking lock-key LEDs.

</br></br>
## $\color{#adf137}\text{Build from source}$ </br>
Requires GCC (MinGW/w64devkit) on Windows.
```
windres lockdisco.rc -O coff -o lockdisco.res
gcc lockdisco.c lockdisco.res -o lockdisco.exe
```




