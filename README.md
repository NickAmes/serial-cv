What is it?
===========
This is the software for my serial-cv board, a small device that generates
control voltages for analog synthesizers. I originally designed it to
fit inside a Korg Monotron, but it may be useful for other synthesizers as well.
For more information, visit the [project page](http://www.fetchmodus.org/projects/serialcv/).

Files
=====
The 'firmware' folder contains the firmware for the device itself.
To compile, you'll need the avr-gcc toolchain. Adjust the Makefile for your
programmer (usbtinyisp by default) and run make.

The 'programs' folder contains a simple library for controlling the
serial-cv board from a computer program (serial-cv.c), a utility to
set the output voltage from a command line (setvoltage.c) and a program
drive a synthesizer's pitch CV from MIDI events (monotron.c).
The serial-cv library and the setvoltage utility should compile on any
POSIX-compliant system (such as GNU/Linux). The monotron program
requires the ALSA Sequencer and libasound, so it will only work
on Linux. Run make to build.

Author and License
==================
This software was written by Nick Ames in 2013.
It is placed in the public domain, in the hope that it will be useful to
someone.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.