# TODO: figure out how to properly generate the list of expected .o files ...

.DEFAULT_GOAL := all

CC=g++
WARNINGS=-W -Wall
WARNINGS+=-Wno-unused-function
WARNINGS+=-Wno-unused-parameter
WARNINGS+=-Wno-unused-const-variable
CFLAGS=-I./src -g $(WARNINGS)

OUTDIR=build
SOURCES=./src/*.cpp
OBJS3=\
	$(OUTDIR)/main.o \
  $(OUTDIR)/config.o \
  $(OUTDIR)/io.o \
  $(OUTDIR)/log.o \
  $(OUTDIR)/mem.o \
  $(OUTDIR)/pool.o \
  $(OUTDIR)/term.o \
  $(OUTDIR)/textbuffer.o
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
$(OUTDIR)/%.o: src/%.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

$(OUTDIR)/$(EXEC): $(OBJS3)
	$(CC) $(CFLAGS) -o $@ $(OBJS3)
