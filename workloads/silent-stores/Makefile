# default to processing all files; allow overwrite by env variable
_OBJ ?= silent-stores victim victim_short induce_scanning memory_checker server client server_files covert covert_send covert_sodium sodium_test2
GEMPATH ?= /shared/pshome2/silent_stores_gem5
SODIUM_DIR ?= libsodium_min

ODIR=bin
OBJ = $(_OBJ:%=$(ODIR)/%.o)
D_OBJ = $(_OBJ:%=$(ODIR)/%.debug)
N_OBJ = $(_OBJ:%=$(ODIR)/%.native)

CPPFLAGS=-O1 --std=c++14 -DGEM5 $(SODIUM_DIR)/src/libsodium/.libs/libsodium.a
CFLAGS=-O1 -DGEM5 $(SODIUM_DIR)/src/libsodium/.libs/libsodium.a
INC=-I$(GEMPATH)/include -I$(SODIUM_DIR)/src/libsodium/include
ASM=$(GEMPATH)/util/m5/m5op_x86.S

all: $(OBJ)

$(ODIR)/%.native: %.cpp
	$(CXX) $(INC) $(ASM) $< $(CPPFLAGS) -o $@
$(ODIR)/%.debug: %.cpp
	$(CXX) $(INC) $(ASM) $< $(CPPFLAGS) -o $@
$(ODIR)/%.o: %.cpp
	$(CXX) $(INC) $(ASM) $< $(CPPFLAGS) -o $@

$(ODIR)/%.native: %.c
	$(CC) $(INC) $(ASM) $< $(CFLAGS) -o $@
$(ODIR)/%.debug: %.c
	$(CC) $(INC) $(ASM) $< $(CFLAGS) -o $@
$(ODIR)/%.o: %.c
	$(CC) $(INC) $(ASM) $< $(CFLAGS) -o $@

$(OBJ): |$(ODIR)

.PHONY: clean

debug: CPPFLAGS := $(filter-out -DGEM5,$(CPPFLAGS))
debug: CPPFLAGS += -DDEBUG
debug: CFLAGS := $(filter-out -DGEM5,$(CFLAGS))
debug: CFLAGS += -DDEBUG
debug: $(D_OBJ)

native: CPPFLAGS := $(filter-out -DGEM5,$(CPPFLAGS))
native: CFLAGS := $(filter-out -DGEM5,$(CFLAGS))
native: $(N_OBJ)

$(ODIR):
	mkdir -p $(ODIR)

dclean:
	rm -f $(D_OBJ)
nclean:
	rm -f $(N_OBJ)
clean:
	rm -f $(OBJ) $(D_OBJ) $(N_OBJ)
