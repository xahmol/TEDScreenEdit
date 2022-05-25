# TED Screen Editor:
# Screen editor for the Plus/4
# Written in 2022 by Xander Mol
# https://github.com/xahmol/VDCScreenEdit
# https://www.idreamtin8bits.com/

# See src/main.c for full credits

# Prerequisites for building:
# - CC65 compiled and included in path with sudo make avail
# - ZIP packages installed: sudo apt-get install zip

SOURCESMAIN = src/main.c src/ted_core.c
SOURCESGEN = src/prggenerator.c
SOURCESLIB = src/ted_core_assembly.s src/visualpetscii.s
GENLIB = src/prggenerate.s
OBJECTS = tedse.tscr.prg tedse.hsc1.prg tedse.hsc2.prg tedse.hsc3.prg tedse.hsc4.prg tedse.petv.prg tedse2prg.ass.prg

ZIP = tedscreenedit-v099-$(shell date "+%Y%m%d-%H%M").zip
D64 = tedse.d64
D81 = tedse.d81
README = README.pdf

MAIN = tedse.prg
GEN = tedse2prggcode.prg
GENPACKED = tedse2prg.prg

CC65_TARGET = plus4
CC = cl65
CFLAGS  = -t $(CC65_TARGET) --create-dep $(<:.c=.d) -Os -I include
LDFLAGSMAIN = -t $(CC65_TARGET) -C tedse-cc65config.cfg -m $(MAIN).map
LDFLAGSGEN = -t $(CC65_TARGET) -C tedsegen-cc65config.cfg -m $(GEN).map

# Path variables
EXOMIZER = /home/xahmol/exomizer/src/exomizer

# Exomizer parameters
SYSADDRESS = 0x5000
EXOPARAMS = sfx $(SYSADDRESS) -t 4

########################################

.SUFFIXES:
.PHONY: all clean deploy vice
all: $(MAIN) $(GEN) $(GENPACKED) $(D64) $(D81)

ifneq ($(MAKECMDGOALS),clean)
-include $(SOURCESMAIN:.c=.d)
endif

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
  
$(MAIN): $(SOURCESLIB) $(SOURCESMAIN:.c=.o)
	$(CC) $(LDFLAGSMAIN) -o $@ $^

$(GEN): $(GENLIB) $(SOURCESGEN:.c=.o)
	$(CC) $(LDFLAGSGEN) -o $@ $^

$(GENPACKED): $(GEN)
	$(EXOMIZER) $(EXOPARAMS) -o $(GENPACKED) $(GEN)

$(D64):	$(MAIN) $(OBJECTS)
	c1541 -format "tedse,xm" d64 $(D64)
	c1541 -attach $(D64) -write tedse.prg tedse
	c1541 -attach $(D64) -write tedse.tscr.prg tedse.tscr
	c1541 -attach $(D64) -write tedse.hsc1.prg tedse.hsc1
	c1541 -attach $(D64) -write tedse.hsc2.prg tedse.hsc2
	c1541 -attach $(D64) -write tedse.hsc3.prg tedse.hsc3
	c1541 -attach $(D64) -write tedse.hsc4.prg tedse.hsc4
	c1541 -attach $(D64) -write tedse.petv.prg tedse.petv
	c1541 -attach $(D64) -write tedse2prg.prg tedse2prg
	c1541 -attach $(D64) -write tedse2prg.ass.prg tedse2prg.ass

$(D81):	$(MAIN) $(OBJECTS)
	c1541 -format "tedse,xm" d81 $(D81)
	c1541 -attach $(D81) -write tedse.prg tedse
	c1541 -attach $(D81) -write careers.chrs.prg charset
	c1541 -attach $(D81) -write tedse.tscr.prg tedse.tscr
	c1541 -attach $(D81) -write tedse.hsc1.prg tedse.hsc1
	c1541 -attach $(D81) -write tedse.hsc2.prg tedse.hsc2
	c1541 -attach $(D81) -write tedse.hsc3.prg tedse.hsc3
	c1541 -attach $(D81) -write tedse.hsc4.prg tedse.hsc4
	c1541 -attach $(D81) -write tedse.petv.prg tedse.petv
	c1541 -attach $(D81) -write tedse2prg.prg tedse2prg
	c1541 -attach $(D81) -write tedse2prg.ass.prg tedse2prg.ass

#$(ZIP): $(MAIN) $(OBJECTS) $(D64) $(D71) $(D81) $(README)
#	zip $@ $^

clean:
	$(RM) $(SOURCESMAIN:.c=.o) $(SOURCESMAIN:.c=.d) $(MAIN) $(MAIN).map
#	$(RM) $(SOURCESGEN:.c=.o) $(SOURCESGEN:.c=.d) $(GEN) $(GEN).map

# To run software in VICE
vice: $(D81)
	xplus4 -autostart $(D81)
