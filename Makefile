OUTDIR=build

.DEFAULT_GOAL := build

builddir:
	mkdir -p $(OUTDIR)

$(OUTDIR)/chi: chi.c
	gcc -g -o $(OUTDIR)/chi $<

build: builddir $(OUTDIR)/chi

run: build
	$(OUTDIR)/chi 77>>logs

clean:
	rm -rf $(OUTDIR)
