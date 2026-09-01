C77//CYBERAGENDA GFX 3.0
====================

A native 6502 graphical calendar and appointment organizer for the
Commodore 64, with a C77 yellow/black cyberpunk interface.

This is not the original BASIC program with decorative text. Version 3.0
is compiled native code and uses a 320x200 hires bitmap month display, a
relocated custom character set for dialogs, and direct VIC-II drawing.

Version 3.0 adds a Night City terminal opening screen, industrial case
plates, vents, badges, a red CYBERAGENDA wordmark, US federal holidays,
incremental redraws, and brief SID interface beeps.

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

Inside the selected-date panel, press A to add an appointment directly to
that date. Holiday dates show the federal holiday title in this panel.

Red date numbers identify calculated US federal holidays. Red markers are
appointments. The right-side equipment plate shows the active date,
holiday, and appointment data.

DATA AND COMPATIBILITY
----------------------
Appointments are stored inside AGENDA.DAT using the original line-based
event format.
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
