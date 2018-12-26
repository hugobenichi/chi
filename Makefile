OUTDIR=build

.DEFAULT_GOAL := build

builddir:
	mkdir -p $(OUTDIR)

$(OUTDIR)/chi: main.c src/chi/*
	gcc -I./src -v -std=c99 -g -o $(OUTDIR)/chi $<

build: builddir $(OUTDIR)/chi

run: build
	$(OUTDIR)/chi

clean:
	rm -rf $(OUTDIR)
