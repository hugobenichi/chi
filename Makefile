# TODO: figure out how to properly generate the list of expected .o files ...

.DEFAULT_GOAL := all

CC=gcc
WARNINGS=-W -Wall
WARNINGS+=-Wno-initializer-overrides
WARNINGS+=-Wno-unused-function
WARNINGS+=-Wno-unused-parameter
WARNINGS+=-Wno-unused-const-variable
CFLAGS=-I./src -std=c99 -g $(WARNINGS)

OUTDIR=build
SOURCES=./src/chi/*.c
#OBJECTS=$(SOURCES:.c=.o)
#OBJS1=$(patsubst ./src/chi/%.c,$(OUTDIR)/%.o,./src/chi/*.c)
OBJS1=$(patsubst ./src/chi/%.c,$(OUTDIR)/%.o,$(SOURCES))
OBJS3=\
	$(OUTDIR)/main.o \
  $(OUTDIR)/config.o \
  $(OUTDIR)/io.o \
  $(OUTDIR)/log.o \
  $(OUTDIR)/term.o \
  $(OUTDIR)/textbuffer.o \
  $(OUTDIR)/pool.o
OBJECTS=$(OBJS3)
TEST=$(patsubst x%,y%,xa   xb   xc)
EXEC=chi

all: builddir $(OUTDIR)/chi

fullbuild: clean all

run: all
	$(OUTDIR)/$(EXEC)

clean:
	rm -rf $(OUTDIR)

builddir:
	mkdir -p $(OUTDIR)

# TODO: add all header files as sources too !
$(OUTDIR)/%.o: src/chi/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(OUTDIR)/$(EXEC): $(OBJS3)
	$(CC) $(CFLAGS) -o $@ $(OBJS3)
