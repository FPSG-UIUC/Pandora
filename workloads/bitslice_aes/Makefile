CC = gcc
# CFLAGS = -O3 -Wall
CFLAGS=-O1 -Wall -DGEM5 -ggdb

_OBJ ?= test
ODIR=.
G_OBJ = $(_OBJ:%=$(ODIR)/%.o)
D_OBJ = $(_OBJ:%=$(ODIR)/%.debug)
N_OBJ = $(_OBJ:%=$(ODIR)/%.native)

GA_OBJ = $(_OBJ:%=$(ODIR)/%_o.asm)
DA_OBJ = $(_OBJ:%=$(ODIR)/%_debug.asm)
NA_OBJ = $(_OBJ:%=$(ODIR)/%_native.asm)

GEMPATH ?= ../../simulator
INC=-I$(GEMPATH)/include -I$(SODIUM_DIR)/src/libsodium/include
ASM=$(GEMPATH)/util/m5/m5op_x86.S

all: debug native gem

gem: |$(ODIR) $(G_OBJ) $(GA_OBJ)

native: CFLAGS := $(filter-out -DGEM5,$(CFLAGS))
native: |$(ODIR) $(N_OBJ) $(NA_OBJ)

debug: CFLAGS := $(filter-out -DGEM5,$(CFLAGS))
debug: CFLAGS += -DDEBUG
debug: |$(ODIR) $(D_OBJ) $(DA_OBJ)

$(ODIR)/%.o: %.c
	$(CC) $(INC) $(ASM) $(CFLAGS) -o $@ ctaes.c $<

$(ODIR)/%_o.asm: %.o
	objdump -D $< > $@

$(ODIR)/%.native: %.c
	$(CC) $(INC) $(ASM) $(CFLAGS) -o $@ ctaes.c $<

$(ODIR)/%_native.asm: %.native
	objdump -D $< > $@

$(ODIR)/%.debug: %.c
	$(CC) $(INC) $(ASM) $(CFLAGS) -o $@ ctaes.c $<

$(ODIR)/%_debug.asm: %.debug
	objdump -D $< > $@

clean:
	rm *.o
