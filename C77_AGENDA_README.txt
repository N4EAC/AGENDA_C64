C77//AGENDA 1.0
================

A calendar and appointment organizer for the Commodore 64, styled after
the yellow/black cyberpunk industrial design of the Commodore 77.

QUICK START ON C64 ULTIMATE
---------------------------
1. Copy C77_AGENDA.d64 to a USB drive.
2. In the C64 Ultimate menu, mount it in drive 8 with writes enabled.
3. Load and run the first program on the disk:

   LOAD"*",8,1
   RUN

4. Select options with number keys. Dates use YYYYMMDD and times HHMM.
5. Choose SAVE or SAVE + EXIT after making changes.
6. Unmount/eject the disk image cleanly before removing the USB drive.

CONTROLS
--------
1  Monthly calendar. An asterisk marks a date containing an event.
2  Add an event (up to 60 records, 28 characters each).
3  Show events for one date.
4  List every event.
5  Delete an event by its displayed record number.
6  Change the program's current date.
7  Save changes to AGENDA.DAT inside the disk image.
8  Save changes and exit.

The initial date is 20260831. Change it with option 6. The C64 has no
standard real-time clock, so the program does not assume one is present.

COMPATIBILITY
-------------
Designed for standard Commodore 64 BASIC/KERNAL operation and ordinary
1541-compatible disk I/O. It should run on C64 Ultimate, Ultimate 64,
VICE, and original C64 hardware with a compatible writable disk device.

Keep a backup copy of the D64 before storing important appointments.
