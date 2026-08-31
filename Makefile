CC65_ROOT ?= /private/tmp/c77-cc65
CL65 := $(CC65_ROOT)/bin/cl65
C1541 := $(CC65_ROOT)/bin/c1541
PROGRAM := C77_AGENDA_GFX.prg
DISK := C77_AGENDA_GFX.d64

.PHONY: all clean verify

all: $(DISK)

$(PROGRAM): src/agenda.c src/graphics.s
	$(CL65) -t c64 -Oirs -o $@ src/agenda.c src/graphics.s

$(DISK): $(PROGRAM)
	PYTHONDONTWRITEBYTECODE=1 python3 tools/package_gfx.py $(PROGRAM) $@

verify: $(DISK)
	PYTHONDONTWRITEBYTECODE=1 python3 tools/verify_d64.py $(DISK)

clean:
	rm -f $(PROGRAM) $(DISK)
