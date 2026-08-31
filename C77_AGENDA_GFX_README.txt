C77//AGENDA GFX 2.0
====================

A native 6502 graphical calendar and appointment organizer for the
Commodore 64, with a C77 yellow/black cyberpunk interface.

This is not the original BASIC program with decorative text. Version 2.0
is compiled native code and uses a relocated custom character set, custom
panel glyphs, direct screen/color memory drawing, and a hardware sprite
selection frame.

QUICK START ON C64 ULTIMATE
---------------------------
1. Copy C77_AGENDA_GFX.d64 to USB storage.
2. Mount it as writable media in drive 8.
3. Load and run the first program:

   LOAD"*",8,1
   RUN

4. Save before unmounting, and eject the disk image cleanly.

CONTROLS
--------
Joystick port 2 or cursor keys  Move the selected date
Fire or RETURN                   Inspect the selected date
A                                Add an appointment
D                                Delete the first appointment on the date
S                                Save AGENDA.DAT
T                                Set today/current date
L                                Browse every appointment
M                                Advance one month
Q                                Save if modified, then exit

The selected day has a solid yellow cell and animated-looking hardware
sprite frame. Cyan markers identify dates containing appointments. The
right-side DAY FEED shows events for the active date.

DATA AND COMPATIBILITY
----------------------
Appointments are stored in AGENDA.DAT inside the mounted disk image. The
line-based format is compatible with the original C77 Agenda BASIC build.
Up to 60 appointments are retained; descriptions are capped at 28
characters by the data format and at 24 characters by the graphical input
dialog so they fit the panel layout.

The program targets the standard C64 KERNAL, VIC-II, CIA joystick port,
1541-compatible sequential files, and normal 64 KB memory map. It is
intended for C64 Ultimate, Ultimate 64, VICE, and original PAL/NTSC C64s.

Keep a backup copy of the D64 before storing important appointments.

BUILDING
--------
The source is in src/agenda.c and src/graphics.s. Build with cc65:

   make CC65_ROOT=/path/to/cc65
   make verify CC65_ROOT=/path/to/cc65

The original BASIC V2 edition remains available as C77_AGENDA.d64.
