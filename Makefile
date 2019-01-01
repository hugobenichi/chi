# TODO: figure out how to properly generate the list of expected .o files ...

.DEFAULT_GOAL := all

CC=gcc
CFLAGS=-I./src -std=c99 -g

OUTDIR=build
SOURCES=./src/chi/*.c
#OBJECTS=$(SOURCES:.c=.o)
#OBJS1=$(patsubst ./src/chi/%.c,$(OUTDIR)/%.o,./src/chi/*.c)
OBJS1=$(patsubst ./src/chi/%.c,$(OUTDIR)/%.o,$(SOURCES))
OBJS3=$(OUTDIR)/chi.o $(OUTDIR)/config.o $(OUTDIR)/io.o $(OUTDIR)/log.o $(OUTDIR)/term.o
OBJECTS=$(OBJS3)
TEST=$(patsubst x%,y%,xa   xb   xc)
EXEC=chi

all: builddir $(OUTDIR)/chi

run: all
	$(OUTDIR)/$(EXEC)

clean:
	rm -rf $(OUTDIR)

builddir:
	mkdir -p $(OUTDIR)

$(OUTDIR)/%.o: src/chi/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(OUTDIR)/$(EXEC): $(OBJS3)
	$(CC) $(CFLAGS) -o $@ $(OBJS3)
