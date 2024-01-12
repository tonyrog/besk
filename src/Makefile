
OSNAME := $(shell uname -s)

WARN = \
	-Wall \
	-Wcast-align \
	-Wimplicit \
	-Wpointer-arith \
	-Wswitch \
	-Wredundant-decls \
	-Wreturn-type \
	-Wunused

EPX_DIR=$(HOME)/erlang/epx
EPX_REL=$(EPX_DIR)/priv

APP_VSN := $(shell date +'%s')

CC = gcc

EPX_CFLAGS  += $(shell $(EPX_REL)/epx-config --cflags)
EPX_LDFLAGS += $(shell $(EPX_REL)/epx-config --libs)

CFLAGS += -DAPP_VSN=$(APP_VSN)
CFLAGS += -g -MD $(WARN) -I. -DSIMULATOR=1 -DTARGET_UNIX
CFLAGS += $(EPX_CFLAGS)
CFLAGS += -DFONT_DIR=\"$(HOME)/erlang/epx/priv/fonts/\"

BIN = ../bin

TELEX_OBJS = \
	telex.o \
	telex_tool.o

TAPE_OBJS = \
	tape_tool.o

DRUM_OBJS = \
	helord.o \
	drum_tool.o

BESK_TEST_OBJS = \
	helord.o \
	besk_test.o

OBJS = \
	lodepng.o \
	epx_lode_png.o \
	helord.o \
	halvord.o \
	telex.o \
	besk.o \
	besk_sim.o

all: $(BIN)/tape $(BIN)/drum $(BIN)/telex $(BIN)/besk_test $(BIN)/besk

clean:
	rm -rf $(OBJS) $(BESK_TEST_OBJS) $(DRUM_OBJS) $(TAPE_OBJS) $(TELEX_OBJS)

$(BIN)/besk_test: $(BESK_TEST_OBJS)
	$(CC)  $(LDFLAGS) -g -o $@ $(BESK_TEST_OBJS)

$(BIN)/besk: $(OBJS)
	$(CC)  $(LDFLAGS) -g -o $@ $(OBJS) $(EPX_LDFLAGS)

$(BIN)/telex: $(TELEX_OBJS)
	$(CC)  $(LDFLAGS) -g -o $@ $(TELEX_OBJS)

$(BIN)/tape: $(TAPE_OBJS)
	$(CC)  $(LDFLAGS) -g -o $@ $(TAPE_OBJS)

$(BIN)/drum: $(DRUM_OBJS)
	$(CC)  $(LDFLAGS) -g -o $@ $(DRUM_OBJS)

lodepng.o: lodepng.cpp
	$(CC) -xc -c -o $@ -MMD -MF .$<.d $(CFLAGS) $<

epx_lode_png.o: $(EPX_DIR)/c_src/epx_lode_png.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o:	%.c
	$(CC) -c -o $@ -MMD -MF .$<.d $(CFLAGS) $<

-include .*.d
